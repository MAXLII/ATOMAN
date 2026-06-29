param(
    [string]$Port = "COM8",
    [int]$Baud = 921600,
    [int]$AckTimeoutMs = 2000,
    [int]$LongTimeoutSec = 120,
    [switch]$SkipLongSfra
)

$ErrorActionPreference = "Stop"

Set-StrictMode -Version 2.0

$script:Results = New-Object System.Collections.Generic.List[object]

function Add-Result {
    param(
        [string]$Name,
        [bool]$Pass,
        [string]$Detail = ""
    )

    $script:Results.Add([pscustomobject]@{
        Name   = $Name
        Pass   = $Pass
        Detail = $Detail
    }) | Out-Null

    $mark = if ($Pass) { "PASS" } else { "FAIL" }
    Write-Host ("[{0}] {1} {2}" -f $mark, $Name, $Detail)
}

function Get-Crc16Ccitt {
    param([byte[]]$Data)

    [int]$crc = 0xFFFF
    foreach ($b in $Data) {
        $crc = $crc -bxor ([int]$b -shl 8)
        for ($i = 0; $i -lt 8; $i++) {
            if (($crc -band 0x8000) -ne 0) {
                $crc = (($crc -shl 1) -bxor 0x1021) -band 0xFFFF
            } else {
                $crc = ($crc -shl 1) -band 0xFFFF
            }
        }
    }

    return $crc
}

function New-Bytes {
    param([object[]]$Values)

    $list = New-Object System.Collections.Generic.List[byte]
    foreach ($v in $Values) {
        if ($null -eq $v) {
            continue
        }
        if ($v -is [byte[]]) {
            $list.AddRange([byte[]]$v)
        } elseif ($v -is [array]) {
            foreach ($x in $v) {
                $list.Add([byte]$x)
            }
        } else {
            $list.Add([byte]$v)
        }
    }
    return [byte[]]$list.ToArray()
}

function U16Bytes { param([uint16]$Value) return [byte[]][BitConverter]::GetBytes($Value) }
function U32Bytes { param([uint32]$Value) return [byte[]][BitConverter]::GetBytes($Value) }
function I16Bytes { param([int16]$Value) return [byte[]][BitConverter]::GetBytes($Value) }
function F32Bytes { param([single]$Value) return [byte[]][BitConverter]::GetBytes($Value) }

function AsciiBytes {
    param([string]$Text)
    return [byte[]][System.Text.Encoding]::ASCII.GetBytes($Text)
}

function New-Frame {
    param(
        [byte]$CmdSet,
        [byte]$CmdWord,
        [byte[]]$Payload = ([byte[]]@()),
        [byte]$Src = 0x01,
        [byte]$Dst = 0x02,
        [byte]$IsAck = 0x00
    )

    [uint16]$len = [uint16]$Payload.Length
    [byte[]]$body = New-Bytes @(
        0xE8, 0x01,
        $Src, 0x00,
        $Dst, 0x00,
        $CmdSet, $CmdWord, $IsAck,
        ($len -band 0xFF), (($len -shr 8) -band 0xFF),
        $Payload
    )

    [int]$crc = Get-Crc16Ccitt $body
    return New-Bytes @($body, ($crc -band 0xFF), (($crc -shr 8) -band 0xFF), 0x0D, 0x0A)
}

function Read-ByteOrNull {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [datetime]$Deadline
    )

    while ([DateTime]::UtcNow -lt $Deadline) {
        if ($Serial.BytesToRead -gt 0) {
            return [int]$Serial.ReadByte()
        }
        Start-Sleep -Milliseconds 1
    }

    return $null
}

function Read-ProtocolFrame {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [int]$TimeoutMs = 1000
    )

    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    $list = New-Object System.Collections.Generic.List[byte]

    while ([DateTime]::UtcNow -lt $deadline) {
        $b = Read-ByteOrNull -Serial $Serial -Deadline $deadline
        if ($null -eq $b) {
            return $null
        }
        if ([byte]$b -eq 0xE8) {
            $list.Add([byte]$b)
            break
        }
    }

    if ($list.Count -eq 0) {
        return $null
    }

    while (($list.Count -lt 11) -and ([DateTime]::UtcNow -lt $deadline)) {
        $b = Read-ByteOrNull -Serial $Serial -Deadline $deadline
        if ($null -eq $b) { return $null }
        $list.Add([byte]$b)
    }

    if ($list.Count -lt 11) {
        return $null
    }

    [int]$payloadLen = [int]$list[9] + ([int]$list[10] -shl 8)
    [int]$totalLen = 15 + $payloadLen
    while (($list.Count -lt $totalLen) -and ([DateTime]::UtcNow -lt $deadline)) {
        $b = Read-ByteOrNull -Serial $Serial -Deadline $deadline
        if ($null -eq $b) { return $null }
        $list.Add([byte]$b)
    }

    if ($list.Count -lt $totalLen) {
        return $null
    }

    [byte[]]$bytes = [byte[]]$list.ToArray()
    [int]$crcOffset = 11 + $payloadLen
    [uint16]$rxCrc = [uint16]([int]$bytes[$crcOffset] + ([int]$bytes[$crcOffset + 1] -shl 8))
    [byte[]]$crcBody = [byte[]]$bytes[0..($crcOffset - 1)]
    [uint16]$calcCrc = [uint16](Get-Crc16Ccitt $crcBody)
    [bool]$valid = (($rxCrc -eq $calcCrc) -and ($bytes[$crcOffset + 2] -eq 0x0D) -and ($bytes[$crcOffset + 3] -eq 0x0A))

    [byte[]]$payload = [byte[]]@()
    if ($payloadLen -gt 0) {
        $payload = [byte[]]$bytes[11..(10 + $payloadLen)]
    }

    return [pscustomobject]@{
        Bytes      = $bytes
        Valid      = $valid
        Src        = $bytes[2]
        DSrc       = $bytes[3]
        Dst        = $bytes[4]
        DDst       = $bytes[5]
        CmdSet     = $bytes[6]
        CmdWord    = $bytes[7]
        IsAck      = $bytes[8]
        Len        = $payloadLen
        Payload    = $payload
        Crc        = $rxCrc
        CalcCrc    = $calcCrc
    }
}

function Wait-MatchingFrame {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [byte]$CmdSet,
        [byte]$CmdWord,
        [int]$TimeoutMs = 2000,
        [int]$IsAck = 1
    )

    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    while ([DateTime]::UtcNow -lt $deadline) {
        $left = [int]([Math]::Max(20, ($deadline - [DateTime]::UtcNow).TotalMilliseconds))
        $frame = Read-ProtocolFrame -Serial $Serial -TimeoutMs $left
        if ($null -eq $frame) {
            continue
        }
        if (($frame.Valid) -and
            ($frame.CmdSet -eq $CmdSet) -and
            ($frame.CmdWord -eq $CmdWord) -and
            (($IsAck -lt 0) -or ($frame.IsAck -eq $IsAck))) {
            return $frame
        }
    }

    return $null
}

function Invoke-CommandFrame {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [string]$Name,
        [byte]$CmdSet,
        [byte]$CmdWord,
        [byte[]]$Payload = ([byte[]]@()),
        [int]$TimeoutMs = $AckTimeoutMs,
        [int]$ExpectedMinLen = 0
    )

    $Serial.DiscardInBuffer()
    [byte[]]$tx = New-Frame -CmdSet $CmdSet -CmdWord $CmdWord -Payload $Payload
    $Serial.Write($tx, 0, $tx.Length)
    $rx = Wait-MatchingFrame -Serial $Serial -CmdSet $CmdSet -CmdWord $CmdWord -TimeoutMs $TimeoutMs -IsAck 1

    if ($null -eq $rx) {
        Add-Result $Name $false "timeout"
        return $null
    }
    if ($rx.Len -lt $ExpectedMinLen) {
        Add-Result $Name $false ("len={0} < {1}" -f $rx.Len, $ExpectedMinLen)
        return $rx
    }

    Add-Result $Name $true ("len={0}" -f $rx.Len)
    return $rx
}

function U16At { param([byte[]]$P, [int]$Offset) return [BitConverter]::ToUInt16($P, $Offset) }
function U32At { param([byte[]]$P, [int]$Offset) return [BitConverter]::ToUInt32($P, $Offset) }
function F32At { param([byte[]]$P, [int]$Offset) return [BitConverter]::ToSingle($P, $Offset) }

function Get-NamePayload {
    param([string]$Name)

    [byte[]]$nameBytes = AsciiBytes $Name
    return New-Bytes @($nameBytes.Length, $nameBytes)
}

function Decode-ShellName {
    param([byte[]]$Payload, [int]$NameOffset, [int]$NameLen)
    if (($NameLen -le 0) -or (($NameOffset + $NameLen) -gt $Payload.Length)) {
        return ""
    }
    return [System.Text.Encoding]::ASCII.GetString($Payload, $NameOffset, $NameLen)
}

function Test-DemoLoopback {
    param([System.IO.Ports.SerialPort]$Serial)

    [byte[]]$payload = New-Bytes @((U32Bytes 0x12345678), 0xA5, (I16Bytes -123))
    $rx = Invoke-CommandFrame -Serial $Serial -Name "demo.loopback 30/01" -CmdSet 0x30 -CmdWord 0x01 -Payload $payload -ExpectedMinLen 7
    if ($null -ne $rx) {
        $ok = (($rx.Payload.Length -eq 7) -and (($rx.Payload -join ",") -eq ($payload -join ",")))
        Add-Result "demo.loopback payload echo" $ok ("rx={0}" -f (($rx.Payload | ForEach-Object { "{0:X2}" -f $_ }) -join " "))
    }
}

function Test-Shell {
    param([System.IO.Ports.SerialPort]$Serial)

    $rx = Invoke-CommandFrame -Serial $Serial -Name "shell.data_num 01/01" -CmdSet 0x01 -CmdWord 0x01 -ExpectedMinLen 4
    $shellCount = 0
    $counterInfo = $null
    if ($null -ne $rx) {
        $shellCount = [int](U32At $rx.Payload 0)
        Add-Result "shell.data_num count" ($shellCount -gt 0) ("count={0}" -f $shellCount)

        $reports = 0
        $deadline = [DateTime]::UtcNow.AddMilliseconds([Math]::Min(6000, [Math]::Max(1000, $shellCount * 80)))
        while (($reports -lt $shellCount) -and ([DateTime]::UtcNow -lt $deadline)) {
            $f = Wait-MatchingFrame -Serial $Serial -CmdSet 0x01 -CmdWord 0x04 -TimeoutMs 250 -IsAck 0
            if ($null -eq $f) { continue }
            $reports++
            if ($f.Len -ge 14) {
                $nameLen = [int]$f.Payload[0]
                $name = Decode-ShellName -Payload $f.Payload -NameOffset 14 -NameLen $nameLen
                if ($name -eq "DEMO_SHELL_COUNTER") {
                    $counterInfo = [pscustomobject]@{
                        Type = [int]$f.Payload[1]
                        Data = U32At $f.Payload 2
                        Max  = U32At $f.Payload 6
                        Min  = U32At $f.Payload 10
                    }
                }
            }
        }
        Add-Result "shell.report_list 01/04" ($reports -gt 0) ("reports={0}/{1}" -f $reports, $shellCount)
    }

    $rx = Invoke-CommandFrame -Serial $Serial -Name "shell.read DEMO_SHELL_COUNTER 01/02" -CmdSet 0x01 -CmdWord 0x02 -Payload (Get-NamePayload "DEMO_SHELL_COUNTER") -ExpectedMinLen 6
    if ($null -ne $rx) {
        $nameLen = [int]$rx.Payload[0]
        $name = Decode-ShellName -Payload $rx.Payload -NameOffset 6 -NameLen $nameLen
        Add-Result "shell.read name" ($name -eq "DEMO_SHELL_COUNTER") ("name={0}" -f $name)

        if ($null -eq $counterInfo) {
            $counterInfo = [pscustomobject]@{
                Type = [int]$rx.Payload[1]
                Data = U32At $rx.Payload 2
                Max  = 0xFFFFFFFF
                Min  = 0
            }
        }
    }

    if ($null -ne $counterInfo) {
        [byte[]]$writePayload = New-Bytes @(
            (Get-NamePayload "DEMO_SHELL_COUNTER"),
            (U32Bytes ([uint32]$counterInfo.Data)),
            (U32Bytes ([uint32]$counterInfo.Max)),
            (U32Bytes ([uint32]$counterInfo.Min))
        )
        [byte[]]$fixedWritePayload = New-Bytes @(
            [byte]18,
            (U32Bytes ([uint32]$counterInfo.Data)),
            (U32Bytes ([uint32]$counterInfo.Max)),
            (U32Bytes ([uint32]$counterInfo.Min)),
            (AsciiBytes "DEMO_SHELL_COUNTER")
        )
        $null = $writePayload
        $rx = Invoke-CommandFrame -Serial $Serial -Name "shell.write DEMO_SHELL_COUNTER 01/03" -CmdSet 0x01 -CmdWord 0x03 -Payload $fixedWritePayload -ExpectedMinLen 14
    }

    [byte[]]$waveEnable = New-Bytes @([byte]18, [byte]0, (AsciiBytes "DEMO_SHELL_COUNTER"))
    $rx = Invoke-CommandFrame -Serial $Serial -Name "shell.wave_enable 01/05" -CmdSet 0x01 -CmdWord 0x05 -Payload $waveEnable -ExpectedMinLen 1
    if ($null -ne $rx) {
        Add-Result "shell.wave_enable ok" ($rx.Payload[0] -eq 1) ("ok={0}" -f $rx.Payload[0])
    }

    $rx = Invoke-CommandFrame -Serial $Serial -Name "shell.wave_period 01/06" -CmdSet 0x01 -CmdWord 0x06 -Payload (U32Bytes 50) -ExpectedMinLen 4
    if ($null -ne $rx) {
        Add-Result "shell.wave_period value" ((U32At $rx.Payload 0) -eq 50) ("period={0}" -f (U32At $rx.Payload 0))
    }

    $rx = Invoke-CommandFrame -Serial $Serial -Name "shell.wave_start off 01/0C" -CmdSet 0x01 -CmdWord 0x0C -Payload ([byte[]]@(0)) -ExpectedMinLen 0
}

function Test-Scope {
    param([System.IO.Ports.SerialPort]$Serial)

    $Serial.DiscardInBuffer()
    [byte[]]$tx = New-Frame -CmdSet 0x01 -CmdWord 0x18 -Payload ([byte[]]@(0))
    $Serial.Write($tx, 0, $tx.Length)
    $list = Wait-MatchingFrame -Serial $Serial -CmdSet 0x01 -CmdWord 0x18 -TimeoutMs 2000 -IsAck 1
    if ($null -eq $list) {
        Add-Result "scope.list 01/18" $false "timeout"
        return
    }
    $scopeId = [int]$list.Payload[0]
    $scopeName = ""
    if ($list.Len -ge 4) {
        $scopeName = Decode-ShellName -Payload $list.Payload -NameOffset 4 -NameLen ([int]$list.Payload[2])
    }
    Add-Result "scope.list 01/18" (($list.Valid) -and ($scopeId -ne 0xFF)) ("id={0} name={1}" -f $scopeId, $scopeName)

    [byte[]]$idPayload = New-Bytes @([byte]$scopeId, 0, 0, 0)
    $info = Invoke-CommandFrame -Serial $Serial -Name "scope.info 01/19" -CmdSet 0x01 -CmdWord 0x19 -Payload $idPayload -ExpectedMinLen 32
    $varCount = 0
    $sampleCount = 0
    $captureTag = 0
    if ($null -ne $info) {
        $varCount = [int]$info.Payload[4]
        $sampleCount = [int](U32At $info.Payload 8)
        $captureTag = [uint32](U32At $info.Payload 28)
        Add-Result "scope.info fields" (($info.Payload[1] -eq 0) -and ($varCount -gt 0) -and ($sampleCount -gt 0)) ("vars={0} samples={1} tag={2}" -f $varCount, $sampleCount, $captureTag)
    }

    for ($i = 0; $i -lt [Math]::Min(2, $varCount); $i++) {
        [byte[]]$vp = New-Bytes @([byte]$scopeId, [byte]$i, 0, 0)
        $v = Invoke-CommandFrame -Serial $Serial -Name ("scope.var[{0}] 01/1A" -f $i) -CmdSet 0x01 -CmdWord 0x1A -Payload $vp -ExpectedMinLen 8
        if ($null -ne $v) {
            $name = Decode-ShellName -Payload $v.Payload -NameOffset 8 -NameLen ([int]$v.Payload[4])
            Add-Result ("scope.var[{0}] name" -f $i) ($v.Payload[1] -eq 0) ("name={0}" -f $name)
        }
    }

    $null = Invoke-CommandFrame -Serial $Serial -Name "scope.reset 01/1E" -CmdSet 0x01 -CmdWord 0x1E -Payload ([byte[]]@([byte]$scopeId)) -ExpectedMinLen 8
    $null = Invoke-CommandFrame -Serial $Serial -Name "scope.start 01/1B" -CmdSet 0x01 -CmdWord 0x1B -Payload ([byte[]]@([byte]$scopeId)) -ExpectedMinLen 8
    Start-Sleep -Milliseconds 100
    $null = Invoke-CommandFrame -Serial $Serial -Name "scope.trigger 01/1C" -CmdSet 0x01 -CmdWord 0x1C -Payload ([byte[]]@([byte]$scopeId)) -ExpectedMinLen 8
    Start-Sleep -Milliseconds 200
    $null = Invoke-CommandFrame -Serial $Serial -Name "scope.stop 01/1D" -CmdSet 0x01 -CmdWord 0x1D -Payload ([byte[]]@([byte]$scopeId)) -ExpectedMinLen 8

    [byte[]]$samplePayload = New-Bytes @([byte]$scopeId, [byte]1, 0, 0, (U32Bytes 0), (U32Bytes 0))
    $sample = Invoke-CommandFrame -Serial $Serial -Name "scope.sample force 01/1F" -CmdSet 0x01 -CmdWord 0x1F -Payload $samplePayload -ExpectedMinLen 16
    if ($null -ne $sample) {
        Add-Result "scope.sample status" ($sample.Payload[1] -eq 0) ("status={0} vars={1}" -f $sample.Payload[1], $sample.Payload[3])
    }
}

function Test-Perf {
    param([System.IO.Ports.SerialPort]$Serial)

    $info = Invoke-CommandFrame -Serial $Serial -Name "perf.info 01/20" -CmdSet 0x01 -CmdWord 0x20 -ExpectedMinLen 20
    if ($null -ne $info) {
        Add-Result "perf.info fields" ((U16At $info.Payload 0) -eq 1) ("records={0} unit_us={1:N3}" -f (U16At $info.Payload 2), (F32At $info.Payload 4))
    }

    $summary = Invoke-CommandFrame -Serial $Serial -Name "perf.summary 01/21" -CmdSet 0x01 -CmdWord 0x21 -ExpectedMinLen 16
    if ($null -ne $summary) {
        Add-Result "perf.summary fields" $true ("task={0:N3}% int={1:N3}%" -f (F32At $summary.Payload 0), (F32At $summary.Payload 8))
    }

    $reset = Invoke-CommandFrame -Serial $Serial -Name "perf.reset_peak 01/25" -CmdSet 0x01 -CmdWord 0x25 -ExpectedMinLen 4
    if ($null -ne $reset) {
        Add-Result "perf.reset_peak success" ($reset.Payload[0] -eq 1) ("success={0}" -f $reset.Payload[0])
    }

    [byte[]]$dictQuery = New-Bytes @([byte]0, 0, 0, 0, (U32Bytes 0))
    $dict = Invoke-CommandFrame -Serial $Serial -Name "perf.dict_query 01/26" -CmdSet 0x01 -CmdWord 0x26 -Payload $dictQuery -ExpectedMinLen 16
    $dictVersion = 0
    if ($null -ne $dict) {
        $accepted = $dict.Payload[0]
        $recordCount = U16At $dict.Payload 2
        $sequence = U32At $dict.Payload 4
        $dictVersion = U32At $dict.Payload 8
        Add-Result "perf.dict_query accepted" ($accepted -eq 1) ("seq={0} records={1} dict={2}" -f $sequence, $recordCount, $dictVersion)

        $items = 0
        $ended = $false
        $deadline = [DateTime]::UtcNow.AddMilliseconds(6000)
        while ([DateTime]::UtcNow -lt $deadline) {
            $f = Read-ProtocolFrame -Serial $Serial -TimeoutMs 500
            if ($null -eq $f) { continue }
            if (-not $f.Valid -or $f.CmdSet -ne 0x01) { continue }
            if ($f.CmdWord -eq 0x27) { $items++ }
            if ($f.CmdWord -eq 0x28) { $ended = $true; break }
        }
        Add-Result "perf.dict reports 01/27..28" ($ended -and ($items -ge 0)) ("items={0} ended={1}" -f $items, $ended)
    }

    if ($dictVersion -ne 0) {
        [byte[]]$sampleQuery = New-Bytes @([byte]0, [byte]0, 0, 0, (U32Bytes ([uint32]$dictVersion)))
        $sample = Invoke-CommandFrame -Serial $Serial -Name "perf.sample_query 01/29" -CmdSet 0x01 -CmdWord 0x29 -Payload $sampleQuery -ExpectedMinLen 16
        if ($null -ne $sample) {
            Add-Result "perf.sample_query accepted" ($sample.Payload[0] -eq 1) ("records={0} seq={1}" -f (U16At $sample.Payload 2), (U32At $sample.Payload 4))
            $batches = 0
            $ended = $false
            $deadline = [DateTime]::UtcNow.AddMilliseconds(6000)
            while ([DateTime]::UtcNow -lt $deadline) {
                $f = Read-ProtocolFrame -Serial $Serial -TimeoutMs 500
                if ($null -eq $f) { continue }
                if (-not $f.Valid -or $f.CmdSet -ne 0x01) { continue }
                if ($f.CmdWord -eq 0x2A) { $batches++ }
                if ($f.CmdWord -eq 0x2B) { $ended = $true; break }
            }
            Add-Result "perf.sample reports 01/2A..2B" ($ended -and ($batches -ge 0)) ("batches={0} ended={1}" -f $batches, $ended)
        }
    }

    $ctrl = Invoke-CommandFrame -Serial $Serial -Name "perf.report_control off 01/2E" -CmdSet 0x01 -CmdWord 0x2E -Payload ([byte[]]@(0)) -ExpectedMinLen 1
    if ($null -ne $ctrl) {
        Add-Result "perf.report_control success" ($ctrl.Payload[0] -eq 1) ("success={0}" -f $ctrl.Payload[0])
    }
}

function Test-Trace {
    param([System.IO.Ports.SerialPort]$Serial)

    $on = Invoke-CommandFrame -Serial $Serial -Name "trace.control on 01/2C" -CmdSet 0x01 -CmdWord 0x2C -Payload ([byte[]]@(1)) -ExpectedMinLen 4
    if ($null -ne $on) {
        Add-Result "trace.control on fields" (($on.Payload[0] -eq 1) -and ($on.Payload[1] -eq 1)) ("unit_us={0}" -f (U16At $on.Payload 2))
    }
    Start-Sleep -Milliseconds 100
    $off = Invoke-CommandFrame -Serial $Serial -Name "trace.control off 01/2C" -CmdSet 0x01 -CmdWord 0x2C -Payload ([byte[]]@(0)) -ExpectedMinLen 4
    if ($null -ne $off) {
        Add-Result "trace.control off fields" (($off.Payload[0] -eq 1) -and ($off.Payload[1] -eq 0)) ("unit_us={0}" -f (U16At $off.Payload 2))
    }
}

function Test-Sfra {
    param([System.IO.Ports.SerialPort]$Serial)

    $Serial.DiscardInBuffer()
    [byte[]]$tx = New-Frame -CmdSet 0x01 -CmdWord 0x2F -Payload ([byte[]]@(0))
    $Serial.Write($tx, 0, $tx.Length)
    $list = Wait-MatchingFrame -Serial $Serial -CmdSet 0x01 -CmdWord 0x2F -TimeoutMs 2000 -IsAck 1
    if ($null -eq $list) {
        Add-Result "sfra.list 01/2F" $false "timeout"
        return
    }
    $sfraId = [int]$list.Payload[0]
    $sfraName = Decode-ShellName -Payload $list.Payload -NameOffset 4 -NameLen ([int]$list.Payload[2])
    Add-Result "sfra.list 01/2F" (($sfraId -ne 0xFF) -and ($sfraName.Length -gt 0)) ("id={0} name={1}" -f $sfraId, $sfraName)

    [byte[]]$idPayload = New-Bytes @([byte]$sfraId, 0, 0, 0)
    $info = Invoke-CommandFrame -Serial $Serial -Name "sfra.info 01/30" -CmdSet 0x01 -CmdWord 0x30 -Payload $idPayload -ExpectedMinLen 40
    if ($null -eq $info) { return }

    $freqStart = F32At $info.Payload 28
    $freqEnd = F32At $info.Payload 32
    $amp = F32At $info.Payload 36
    Add-Result "sfra.info fields" ($info.Payload[1] -eq 0) ("len={0} range={1:N1}-{2:N1} amp={3:N3}" -f (U16At $info.Payload 10), $freqStart, $freqEnd, $amp)

    [byte[]]$cfgPayload = New-Bytes @([byte]$sfraId, [byte]0x03, 0, 0, (F32Bytes ([single]$freqStart)), (F32Bytes ([single]$freqEnd)), (F32Bytes ([single]$amp)))
    $cfg = Invoke-CommandFrame -Serial $Serial -Name "sfra.cfg_set same 01/31" -CmdSet 0x01 -CmdWord 0x31 -Payload $cfgPayload -ExpectedMinLen 20
    if ($null -ne $cfg) {
        Add-Result "sfra.cfg_set status" ($cfg.Payload[1] -eq 0) ("status={0}" -f $cfg.Payload[1])
    }

    $reset = Invoke-CommandFrame -Serial $Serial -Name "sfra.reset 01/34" -CmdSet 0x01 -CmdWord 0x34 -Payload $idPayload -ExpectedMinLen 20
    if ($null -ne $reset) {
        Add-Result "sfra.reset status" ($reset.Payload[1] -eq 0) ("status={0} tag={1}" -f $reset.Payload[1], (U32At $reset.Payload 16))
    }

    if ($SkipLongSfra) {
        $pointPayload = New-Bytes @([byte]$sfraId, 0, (U16Bytes 0), (U32Bytes 0))
        $point = Invoke-CommandFrame -Serial $Serial -Name "sfra.point_query not-ready 01/35" -CmdSet 0x01 -CmdWord 0x35 -Payload $pointPayload -ExpectedMinLen 24
        if ($null -ne $point) {
            Add-Result "sfra.point_query not-ready status" ($point.Payload[1] -eq 4) ("status={0}" -f $point.Payload[1])
        }
        return
    }

    $start = Invoke-CommandFrame -Serial $Serial -Name "sfra.start 01/32" -CmdSet 0x01 -CmdWord 0x32 -Payload $idPayload -ExpectedMinLen 20
    if ($null -eq $start) { return }
    Add-Result "sfra.start status" ($start.Payload[1] -eq 0) ("status={0} tag={1}" -f $start.Payload[1], (U32At $start.Payload 16))

    $lastInfo = $null
    $deadline = [DateTime]::UtcNow.AddSeconds($LongTimeoutSec)
    while ([DateTime]::UtcNow -lt $deadline) {
        Start-Sleep -Milliseconds 1000
        $poll = Invoke-CommandFrame -Serial $Serial -Name "sfra.info poll 01/30" -CmdSet 0x01 -CmdWord 0x30 -Payload $idPayload -TimeoutMs 2500 -ExpectedMinLen 40
        if ($null -eq $poll) { continue }
        $lastInfo = $poll
        $idx = U16At $poll.Payload 8
        $len = U16At $poll.Payload 10
        $table = U16At $poll.Payload 12
        $done = $poll.Payload[4]
        Write-Host ("  sfra progress idx={0}/{1} table={2} done={3}" -f $idx, $len, $table, $done)
        if (($done -ne 0) -and ($table -ge $len)) {
            break
        }
    }

    if ($null -eq $lastInfo) {
        Add-Result "sfra.sweep complete" $false "no info"
        return
    }

    $finalIdx = U16At $lastInfo.Payload 8
    $finalLen = U16At $lastInfo.Payload 10
    $finalTable = U16At $lastInfo.Payload 12
    $tag = U32At $lastInfo.Payload 16
    $doneFinal = $lastInfo.Payload[4]
    Add-Result "sfra.sweep complete" (($doneFinal -ne 0) -and ($finalTable -ge $finalLen)) ("idx={0}/{1} table={2} tag={3}" -f $finalIdx, $finalLen, $finalTable, $tag)

    [uint16]$lastPoint = [uint16]([Math]::Max(0, $finalLen - 1))
    foreach ($pi in @([uint16]0, $lastPoint)) {
        [byte[]]$pointPayload = New-Bytes @([byte]$sfraId, 0, (U16Bytes $pi), (U32Bytes ([uint32]$tag)))
        $point = Invoke-CommandFrame -Serial $Serial -Name ("sfra.point_query[{0}] 01/35" -f $pi) -CmdSet 0x01 -CmdWord 0x35 -Payload $pointPayload -ExpectedMinLen 24
        if ($null -ne $point) {
            Add-Result ("sfra.point[{0}] fields" -f $pi) ($point.Payload[1] -eq 0) ("freq={0:N3}Hz mag={1:N6} phase={2:N3}" -f (F32At $point.Payload 12), (F32At $point.Payload 16), (F32At $point.Payload 20))
        }
    }

    $stop = Invoke-CommandFrame -Serial $Serial -Name "sfra.stop 01/33" -CmdSet 0x01 -CmdWord 0x33 -Payload $idPayload -ExpectedMinLen 20
    if ($null -ne $stop) {
        Add-Result "sfra.stop status" ($stop.Payload[1] -eq 0) ("status={0}" -f $stop.Payload[1])
    }
}

$serial = New-Object System.IO.Ports.SerialPort($Port, $Baud, "None", 8, "One")
$serial.ReadTimeout = 100
$serial.WriteTimeout = 1000

Write-Host "Open $Port at $Baud"
$serial.Open()

try {
    Test-DemoLoopback -Serial $serial
    Test-Shell -Serial $serial
    Test-Scope -Serial $serial
    Test-Perf -Serial $serial
    Test-Trace -Serial $serial
    Test-Sfra -Serial $serial
}
finally {
    if ($serial.IsOpen) {
        $serial.Close()
    }
}

$failed = @($script:Results | Where-Object { -not $_.Pass })
$passed = @($script:Results | Where-Object { $_.Pass })
Write-Host ""
Write-Host ("SUMMARY passed={0} failed={1}" -f $passed.Count, $failed.Count)

if ($failed.Count -gt 0) {
    Write-Host "FAILED ITEMS:"
    foreach ($f in $failed) {
        Write-Host (" - {0}: {1}" -f $f.Name, $f.Detail)
    }
    exit 1
}

exit 0
