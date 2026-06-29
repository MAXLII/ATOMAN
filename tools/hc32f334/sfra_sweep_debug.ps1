param(
    [string]$Port = "COM8",
    [int]$Baud = 921600,
    [int]$TimeoutSec = 120,
    [int]$PollMs = 1000,
    [switch]$NoReset,
    [switch]$NoStart,
    [switch]$InfoOnly
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

function New-SfraFrame {
    param(
        [byte]$Cmd,
        [byte[]]$Payload
    )

    $len = $Payload.Length
    [byte[]]$body = @(0xE8, 0x01, 0x01, 0x00, 0x02, 0x00, 0x01, $Cmd, 0x00,
                      ($len -band 0xFF), (($len -shr 8) -band 0xFF)) + $Payload
    [int]$crc = Get-Crc16Ccitt $body
    [byte]$lo = $crc -band 0xFF
    [byte]$hi = ($crc -shr 8) -band 0xFF

    return [byte[]]($body + @($lo, $hi, 0x0D, 0x0A))
}

function Read-SfraFrame {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [int]$TimeoutMs = 1200
    )

    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    $buf = New-Object System.Collections.Generic.List[byte]

    while ([DateTime]::UtcNow -lt $deadline) {
        while ($Serial.BytesToRead -gt 0) {
            [byte]$b = $Serial.ReadByte()
            if (($buf.Count -eq 0) -and ($b -ne 0xE8)) {
                continue
            }

            $buf.Add($b)
            $n = $buf.Count
            if (($n -ge 15) -and ($buf[$n - 2] -eq 0x0D) -and ($buf[$n - 1] -eq 0x0A)) {
                return [byte[]]$buf.ToArray()
            }
        }

        Start-Sleep -Milliseconds 2
    }

    return [byte[]]$buf.ToArray()
}

function Send-SfraCommand {
    param(
        [System.IO.Ports.SerialPort]$Serial,
        [byte]$Cmd,
        [byte[]]$Payload = ([byte[]]@(0, 0, 0, 0))
    )

    [byte[]]$frame = New-SfraFrame -Cmd $Cmd -Payload $Payload
    $Serial.DiscardInBuffer()
    $Serial.Write($frame, 0, $frame.Length)

    $deadline = [DateTime]::UtcNow.AddMilliseconds(2000)
    do {
        [byte[]]$rx = Read-SfraFrame -Serial $Serial -TimeoutMs 300
        if (($rx.Length -ge 15) -and ($rx[7] -eq $Cmd) -and ($rx[8] -eq 1)) {
            return $rx
        }
    } while ([DateTime]::UtcNow -lt $deadline)

    return [byte[]]@()
}

function Convert-InfoAck {
    param([byte[]]$Frame)

    if (($null -eq $Frame) -or ($Frame.Length -lt 55)) {
        return $null
    }

    $len = [int]$Frame[9] + ([int]$Frame[10] -shl 8)
    if (($len -lt 40) -or ($Frame.Length -lt (11 + $len + 4))) {
        return $null
    }

    $p = $Frame[11..(10 + $len)]
    return [pscustomobject]@{
        Id          = [int]$p[0]
        Status      = [int]$p[1]
        State       = [int]$p[2]
        Busy        = [int]$p[3]
        Done        = [int]$p[4]
        DataReady   = [int]$p[5]
        Index       = [BitConverter]::ToUInt16($p, 8)
        Length      = [BitConverter]::ToUInt16($p, 10)
        TableLength = [BitConverter]::ToUInt16($p, 12)
        SweepTag    = [BitConverter]::ToUInt32($p, 16)
        CurrentFreq = [BitConverter]::ToSingle($p, 20)
        IsrFreq     = [BitConverter]::ToSingle($p, 24)
    }
}

function New-PointPayload {
    param(
        [uint16]$Index,
        [uint32]$SweepTag
    )

    return [byte[]]@(
        0, 0,
        ($Index -band 0xFF), (($Index -shr 8) -band 0xFF),
        ($SweepTag -band 0xFF), (($SweepTag -shr 8) -band 0xFF),
        (($SweepTag -shr 16) -band 0xFF), (($SweepTag -shr 24) -band 0xFF)
    )
}

function Convert-PointAck {
    param([byte[]]$Frame)

    if (($null -eq $Frame) -or ($Frame.Length -lt 35)) {
        return $null
    }

    $len = [int]$Frame[9] + ([int]$Frame[10] -shl 8)
    if (($len -lt 24) -or ($Frame.Length -lt (11 + $len + 4))) {
        return $null
    }

    $p = $Frame[11..(10 + $len)]
    return [pscustomobject]@{
        Status     = [int]$p[1]
        IsLast     = [int]$p[2]
        PointIndex = [BitConverter]::ToUInt16($p, 4)
        PointCount = [BitConverter]::ToUInt16($p, 6)
        SweepTag   = [BitConverter]::ToUInt32($p, 8)
        FreqHz     = [BitConverter]::ToSingle($p, 12)
        Mag        = [BitConverter]::ToSingle($p, 16)
        PhaseDeg   = [BitConverter]::ToSingle($p, 20)
    }
}

$serial = New-Object System.IO.Ports.SerialPort($Port, $Baud, "None", 8, "One")
$serial.ReadTimeout = 100
$serial.WriteTimeout = 1000

Write-Host "Open $Port at $Baud"
$serial.Open()

try {
    if (-not $NoReset) {
        [void](Send-SfraCommand -Serial $serial -Cmd 0x34)
        Start-Sleep -Milliseconds 100
    }

    if (-not $NoStart) {
        $startAck = Send-SfraCommand -Serial $serial -Cmd 0x32
        Write-Host ("Start ACK length: {0}" -f $startAck.Length)
    }

    if ($InfoOnly) {
        $info = Convert-InfoAck (Send-SfraCommand -Serial $serial -Cmd 0x30)
        if ($null -eq $info) {
            throw "Info query failed."
        }

        Write-Host ("idx={0}/{1} table={2} status={3} state={4} busy={5} done={6} tag={7} freq={8:N3}Hz isr={9:N1}Hz" -f `
            $info.Index, $info.Length, $info.TableLength, $info.Status, $info.State, `
            $info.Busy, $info.Done, $info.SweepTag, $info.CurrentFreq, $info.IsrFreq)
        exit 0
    }

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSec)
    $lastIndex = -1
    $lastInfo = $null

    while ([DateTime]::UtcNow -lt $deadline) {
        Start-Sleep -Milliseconds $PollMs
        $info = Convert-InfoAck (Send-SfraCommand -Serial $serial -Cmd 0x30)
        if ($null -eq $info) {
            Write-Host "Info timeout or invalid frame"
            continue
        }

        $lastInfo = $info
        if (($info.Index -ne $lastIndex) -or ($info.Done -ne 0)) {
            Write-Host ("idx={0}/{1} table={2} status={3} state={4} busy={5} done={6} tag={7} freq={8:N3}Hz" -f `
                $info.Index, $info.Length, $info.TableLength, $info.Status, $info.State, `
                $info.Busy, $info.Done, $info.SweepTag, $info.CurrentFreq)
            $lastIndex = $info.Index
        }

        if (($info.Done -ne 0) -and ($info.TableLength -ge $info.Length)) {
            break
        }
    }

    if ($null -eq $lastInfo) {
        throw "No valid SFRA info frame received."
    }

    if (($lastInfo.Done -eq 0) -or ($lastInfo.TableLength -lt $lastInfo.Length)) {
        throw ("SFRA did not complete: idx={0}/{1}, table={2}, done={3}" -f `
            $lastInfo.Index, $lastInfo.Length, $lastInfo.TableLength, $lastInfo.Done)
    }

    foreach ($pointIndex in @(0, [Math]::Max(0, $lastInfo.Length - 1))) {
        $payload = New-PointPayload -Index ([uint16]$pointIndex) -SweepTag ([uint32]$lastInfo.SweepTag)
        $point = Convert-PointAck (Send-SfraCommand -Serial $serial -Cmd 0x35 -Payload $payload)
        if ($null -eq $point) {
            throw "Point query failed for index $pointIndex."
        }
        if ($point.Status -ne 0) {
            throw ("Point query returned status {0} for index {1}." -f $point.Status, $pointIndex)
        }

        Write-Host ("point={0} status={1} last={2} count={3} tag={4} freq={5:N3}Hz mag={6:N6} phase={7:N3}deg" -f `
            $point.PointIndex, $point.Status, $point.IsLast, $point.PointCount, `
            $point.SweepTag, $point.FreqHz, $point.Mag, $point.PhaseDeg)
    }

    Write-Host "SFRA sweep debug passed."
    exit 0
}
finally {
    if ($serial.IsOpen) {
        $serial.Close()
    }
}
