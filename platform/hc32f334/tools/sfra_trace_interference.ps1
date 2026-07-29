param(
    [string]$Port = "COM8",
    [int]$Baud = 921600,
    [int]$TimeoutSec = 40
)

$ErrorActionPreference = "Stop"

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

function New-Frame {
    param(
        [byte]$CmdSet,
        [byte]$CmdWord,
        [byte[]]$Payload = ([byte[]]@())
    )

    [uint16]$len = [uint16]$Payload.Length
    [byte[]]$body = New-Bytes @(
        0xE8, 0x01,
        0x01, 0x00,
        0x02, 0x00,
        $CmdSet, $CmdWord, 0x00,
        ($len -band 0xFF), (($len -shr 8) -band 0xFF),
        $Payload
    )

    [int]$crc = Get-Crc16Ccitt $body
    return New-Bytes @($body, ($crc -band 0xFF), (($crc -shr 8) -band 0xFF), 0x0D, 0x0A)
}

function Read-Frame {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [int]$TimeoutMs = 1000
    )

    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    $list = New-Object System.Collections.Generic.List[byte]

    while ([DateTime]::UtcNow -lt $deadline) {
        if ($Serial.BytesToRead -le 0) {
            Start-Sleep -Milliseconds 1
            continue
        }

        [byte]$b = $Serial.ReadByte()
        if (($list.Count -eq 0) -and ($b -ne 0xE8)) {
            continue
        }

        $list.Add($b)
        if ($list.Count -eq 11) {
            $payloadLen = [int]$list[9] + ([int]$list[10] -shl 8)
            $script:ExpectedFrameLen = 15 + $payloadLen
        }

        if (($list.Count -ge 11) -and ($list.Count -ge $script:ExpectedFrameLen)) {
            [byte[]]$bytes = [byte[]]$list.ToArray()
            $len = [int]$bytes[9] + ([int]$bytes[10] -shl 8)
            [byte[]]$payload = [byte[]]@()
            if ($len -gt 0) {
                $payload = [byte[]]$bytes[11..(10 + $len)]
            }

            return [pscustomobject]@{
                Bytes = $bytes
                CmdSet = $bytes[6]
                CmdWord = $bytes[7]
                IsAck = $bytes[8]
                Len = $len
                Payload = $payload
            }
        }
    }

    return $null
}

function Wait-Ack {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [byte]$CmdSet,
        [byte]$CmdWord,
        [int]$TimeoutMs = 2000
    )

    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    while ([DateTime]::UtcNow -lt $deadline) {
        $left = [int][Math]::Max(50, ($deadline - [DateTime]::UtcNow).TotalMilliseconds)
        $f = Read-Frame -Serial $Serial -TimeoutMs $left
        if ($null -eq $f) {
            continue
        }
        if (($f.CmdSet -eq $CmdSet) -and ($f.CmdWord -eq $CmdWord) -and ($f.IsAck -eq 1)) {
            return $f
        }
    }

    return $null
}

function Send-Command {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [string]$Name,
        [byte]$CmdSet,
        [byte]$CmdWord,
        [byte[]]$Payload = ([byte[]]@()),
        [int]$TimeoutMs = 2000
    )

    [byte[]]$tx = New-Frame -CmdSet $CmdSet -CmdWord $CmdWord -Payload $Payload
    $Serial.Write($tx, 0, $tx.Length)
    $ack = Wait-Ack -Serial $Serial -CmdSet $CmdSet -CmdWord $CmdWord -TimeoutMs $TimeoutMs
    if ($null -eq $ack) {
        Write-Host ("{0}: timeout" -f $Name)
    } else {
        Write-Host ("{0}: ack len={1}" -f $Name, $ack.Len)
    }
    return $ack
}

function U16At { param([byte[]]$P, [int]$Offset) return [BitConverter]::ToUInt16($P, $Offset) }
function U32At { param([byte[]]$P, [int]$Offset) return [BitConverter]::ToUInt32($P, $Offset) }
function F32At { param([byte[]]$P, [int]$Offset) return [BitConverter]::ToSingle($P, $Offset) }

function Print-SfraInfo {
    param([string]$Label, [object]$Frame)

    if (($null -eq $Frame) -or ($Frame.Len -lt 40)) {
        Write-Host ("{0}: invalid" -f $Label)
        return
    }

    $p = $Frame.Payload
    Write-Host ("{0}: status={1} state={2} busy={3} done={4} data={5} idx={6}/{7} table={8} tag={9} freq={10:N3}Hz" -f `
        $Label, $p[1], $p[2], $p[3], $p[4], $p[5], (U16At $p 8), (U16At $p 10), `
        (U16At $p 12), (U32At $p 16), (F32At $p 20))
}

$serial = New-Object System.IO.Ports.SerialPort($Port, $Baud, "None", 8, "One")
$serial.ReadTimeout = 100
$serial.WriteTimeout = 1000

Write-Host "Open $Port at $Baud"
$serial.Open()

try {
    $serial.DiscardInBuffer()

    [byte[]]$idPayload = [byte[]]@(0, 0, 0, 0)
    [void](Send-Command -Serial $serial -Name "sfra.reset" -CmdSet 0x01 -CmdWord 0x34 -Payload $idPayload)
    Start-Sleep -Milliseconds 100
    [void](Send-Command -Serial $serial -Name "sfra.start" -CmdSet 0x01 -CmdWord 0x32 -Payload $idPayload)

    for ($i = 0; $i -lt 3; $i++) {
        Start-Sleep -Milliseconds 1000
        $info = Send-Command -Serial $serial -Name ("sfra.info before_trace[{0}]" -f $i) -CmdSet 0x01 -CmdWord 0x30 -Payload $idPayload
        Print-SfraInfo -Label ("before_trace[{0}]" -f $i) -Frame $info
    }

    [void](Send-Command -Serial $serial -Name "trace.on" -CmdSet 0x01 -CmdWord 0x2C -Payload ([byte[]]@(1)))

    for ($i = 0; $i -lt 5; $i++) {
        Start-Sleep -Milliseconds 1000
        $info = Send-Command -Serial $serial -Name ("sfra.info trace_on[{0}]" -f $i) -CmdSet 0x01 -CmdWord 0x30 -Payload $idPayload
        Print-SfraInfo -Label ("trace_on[{0}]" -f $i) -Frame $info
    }

    [void](Send-Command -Serial $serial -Name "trace.off" -CmdSet 0x01 -CmdWord 0x2C -Payload ([byte[]]@(0)))

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSec)
    $lastIdx = 65535
    while ([DateTime]::UtcNow -lt $deadline) {
        Start-Sleep -Milliseconds 1000
        $info = Send-Command -Serial $serial -Name "sfra.info after_trace" -CmdSet 0x01 -CmdWord 0x30 -Payload $idPayload -TimeoutMs 2500
        Print-SfraInfo -Label "after_trace" -Frame $info
        if (($null -ne $info) -and ($info.Len -ge 40)) {
            $idx = U16At $info.Payload 8
            $len = U16At $info.Payload 10
            $table = U16At $info.Payload 12
            $done = $info.Payload[4]
            if (($done -ne 0) -and ($table -ge $len)) {
                break
            }
            if ($idx -eq $lastIdx) {
                Write-Host ("progress stalled at idx={0}" -f $idx)
            }
            $lastIdx = $idx
        }
    }
}
finally {
    if ($serial.IsOpen) {
        $serial.Close()
    }
}
