param(
    [string]$Port = "COM8",
    [int]$Baud = 921600,
    [int]$Rounds = 12
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
        if ($null -eq $v) { continue }
        if ($v -is [byte[]]) {
            $list.AddRange([byte[]]$v)
        } elseif ($v -is [array]) {
            foreach ($x in $v) { $list.Add([byte]$x) }
        } else {
            $list.Add([byte]$v)
        }
    }
    return [byte[]]$list.ToArray()
}

function U32Bytes { param([uint32]$Value) return [byte[]][BitConverter]::GetBytes($Value) }

function New-Frame {
    param([byte]$CmdWord, [byte[]]$Payload = ([byte[]]@()))
    [uint16]$len = [uint16]$Payload.Length
    [byte[]]$body = New-Bytes @(0xE8, 0x01, 0x01, 0x00, 0x02, 0x00, 0x01, $CmdWord, 0x00,
        ($len -band 0xFF), (($len -shr 8) -band 0xFF), $Payload)
    [int]$crc = Get-Crc16Ccitt $body
    return New-Bytes @($body, ($crc -band 0xFF), (($crc -shr 8) -band 0xFF), 0x0D, 0x0A)
}

function Read-Frame {
    param([System.IO.Ports.SerialPort]$Serial, [int]$TimeoutMs = 1000)
    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    $list = New-Object System.Collections.Generic.List[byte]
    $expected = 0
    while ([DateTime]::UtcNow -lt $deadline) {
        if ($Serial.BytesToRead -le 0) { Start-Sleep -Milliseconds 1; continue }
        [byte]$b = $Serial.ReadByte()
        if (($list.Count -eq 0) -and ($b -ne 0xE8)) { continue }
        $list.Add($b)
        if ($list.Count -eq 11) {
            $expected = 15 + [int]$list[9] + ([int]$list[10] -shl 8)
        }
        if (($expected -gt 0) -and ($list.Count -ge $expected)) {
            [byte[]]$bytes = [byte[]]$list.ToArray()
            $len = [int]$bytes[9] + ([int]$bytes[10] -shl 8)
            [byte[]]$payload = [byte[]]@()
            if ($len -gt 0) { $payload = [byte[]]$bytes[11..(10 + $len)] }
            return [pscustomobject]@{
                CmdWord = $bytes[7]
                IsAck = $bytes[8]
                Len = $len
                Payload = $payload
            }
        }
    }
    return $null
}

function Send-Command {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [string]$Name,
        [byte]$CmdWord,
        [byte[]]$Payload = ([byte[]]@()),
        [int]$TimeoutMs = 2000
    )
    [byte[]]$tx = New-Frame -CmdWord $CmdWord -Payload $Payload
    $Serial.Write($tx, 0, $tx.Length)
    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    while ([DateTime]::UtcNow -lt $deadline) {
        $f = Read-Frame -Serial $Serial -TimeoutMs 200
        if (($null -ne $f) -and ($f.CmdWord -eq $CmdWord) -and ($f.IsAck -eq 1)) {
            Write-Host ("{0}: ack len={1}" -f $Name, $f.Len)
            return $f
        }
    }
    Write-Host ("{0}: timeout" -f $Name)
    return $null
}

function U16At { param([byte[]]$P, [int]$Offset) return [BitConverter]::ToUInt16($P, $Offset) }
function U32At { param([byte[]]$P, [int]$Offset) return [BitConverter]::ToUInt32($P, $Offset) }
function F32At { param([byte[]]$P, [int]$Offset) return [BitConverter]::ToSingle($P, $Offset) }

function Print-SfraInfo {
    param([string]$Label, [object]$Frame)
    if (($null -eq $Frame) -or ($Frame.Len -lt 40)) { Write-Host "$Label invalid"; return }
    $p = $Frame.Payload
    Write-Host ("{0}: state={1} busy={2} done={3} idx={4}/{5} table={6} freq={7:N3}Hz" -f `
        $Label, $p[2], $p[3], $p[4], (U16At $p 8), (U16At $p 10), (U16At $p 12), (F32At $p 20))
}

function Print-SfraPoint {
    param([string]$Label, [object]$Frame)
    if (($null -eq $Frame) -or ($Frame.Len -lt 24)) { Write-Host "$Label invalid"; return }
    $p = $Frame.Payload
    Write-Host ("{0}: status={1} point={2}/{3} tag={4} freq={5:N3}Hz mag={6:N6} phase={7:N3}deg" -f `
        $Label, $p[1], (U16At $p 4), (U16At $p 6), (U32At $p 8), (F32At $p 12), (F32At $p 16), (F32At $p 20))
}

$serial = New-Object System.IO.Ports.SerialPort($Port, $Baud, "None", 8, "One")
$serial.ReadTimeout = 100
$serial.WriteTimeout = 1000

Write-Host "Open $Port at $Baud"
$serial.Open()

try {
    $serial.DiscardInBuffer()
    [byte[]]$id = [byte[]]@(0, 0, 0, 0)
    [void](Send-Command -Serial $serial -Name "sfra.reset" -CmdWord 0x34 -Payload $id)
    [void](Send-Command -Serial $serial -Name "scope.reset" -CmdWord 0x1E -Payload ([byte[]]@(0)))
    [void](Send-Command -Serial $serial -Name "sfra.start" -CmdWord 0x32 -Payload $id)
    [void](Send-Command -Serial $serial -Name "scope.start" -CmdWord 0x1B -Payload ([byte[]]@(0)))
    Start-Sleep -Milliseconds 200
    [void](Send-Command -Serial $serial -Name "scope.trigger" -CmdWord 0x1C -Payload ([byte[]]@(0)))

    for ($i = 0; $i -lt $Rounds; $i++) {
        Start-Sleep -Milliseconds 500
        $info = Send-Command -Serial $serial -Name "sfra.info" -CmdWord 0x30 -Payload $id -TimeoutMs 2500
        Print-SfraInfo -Label ("round[{0}]" -f $i) -Frame $info

        if (($null -ne $info) -and ($info.Len -ge 20)) {
            $tableLength = U16At $info.Payload 12
            $sweepTag = U32At $info.Payload 16
            if (($tableLength -gt 0) -and (($i % 20) -eq 0)) {
                [uint16]$pointIndex = [uint16]($tableLength - 1)
                [byte[]]$pointPayload = New-Bytes @([byte]0, [byte]0,
                    ($pointIndex -band 0xFF), (($pointIndex -shr 8) -band 0xFF), (U32Bytes $sweepTag))
                $point = Send-Command -Serial $serial -Name ("sfra.point[{0}]" -f $pointIndex) -CmdWord 0x35 -Payload $pointPayload -TimeoutMs 2500
                Print-SfraPoint -Label ("point[{0}]" -f $pointIndex) -Frame $point
            }
        }

        [byte[]]$samplePayload = New-Bytes @([byte]0, [byte]1, 0, 0, (U32Bytes ([uint32]($i % 100))), (U32Bytes 0))
        [void](Send-Command -Serial $serial -Name "scope.sample.force" -CmdWord 0x1F -Payload $samplePayload -TimeoutMs 2500)
    }
}
finally {
    if ($serial.IsOpen) { $serial.Close() }
}
