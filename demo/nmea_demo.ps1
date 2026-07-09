param(
    [string]$HostName = "127.0.0.1",
    [int]$Port = 10110,
    [int]$IntervalMs = 1000,
    [int]$DurationSeconds = 600,
    [double]$SpeedKnots = 14.2
)

$ErrorActionPreference = "Stop"

function ConvertTo-NmeaCoordinate {
    param(
        [double]$DecimalDegrees,
        [ValidateSet("Latitude", "Longitude")]
        [string]$Kind
    )

    $absolute = [Math]::Abs($DecimalDegrees)
    $degrees = [Math]::Floor($absolute)
    $minutes = ($absolute - $degrees) * 60.0

    if ($Kind -eq "Latitude") {
        $value = "{0:00}{1:00.0000}" -f [int]$degrees, $minutes
        $hemisphere = if ($DecimalDegrees -ge 0) { "N" } else { "S" }
    } else {
        $value = "{0:000}{1:00.0000}" -f [int]$degrees, $minutes
        $hemisphere = if ($DecimalDegrees -ge 0) { "E" } else { "W" }
    }

    return @{
        Value = $value
        Hemisphere = $hemisphere
    }
}

function Get-NmeaChecksum {
    param([string]$Body)

    $checksum = 0
    foreach ($character in $Body.ToCharArray()) {
        $checksum = $checksum -bxor [byte][char]$character
    }

    return "{0:X2}" -f $checksum
}

function Get-CourseDegrees {
    param(
        [double]$FromLat,
        [double]$FromLon,
        [double]$ToLat,
        [double]$ToLon
    )

    $lat1 = $FromLat * [Math]::PI / 180.0
    $lat2 = $ToLat * [Math]::PI / 180.0
    $deltaLon = ($ToLon - $FromLon) * [Math]::PI / 180.0

    $y = [Math]::Sin($deltaLon) * [Math]::Cos($lat2)
    $x = [Math]::Cos($lat1) * [Math]::Sin($lat2) -
        [Math]::Sin($lat1) * [Math]::Cos($lat2) * [Math]::Cos($deltaLon)

    $bearing = [Math]::Atan2($y, $x) * 180.0 / [Math]::PI
    return ($bearing + 360.0) % 360.0
}

function New-RmcSentence {
    param(
        [double]$Latitude,
        [double]$Longitude,
        [double]$SogKnots,
        [double]$CourseDegrees,
        [datetime]$Timestamp
    )

    $lat = ConvertTo-NmeaCoordinate -DecimalDegrees $Latitude -Kind Latitude
    $lon = ConvertTo-NmeaCoordinate -DecimalDegrees $Longitude -Kind Longitude
    $utc = $Timestamp.ToUniversalTime()
    $timeText = $utc.ToString("HHmmss") + ".00"
    $dateText = $utc.ToString("ddMMyy")
    $sogText = "{0:0.0}" -f $SogKnots
    $courseText = "{0:0.0}" -f $CourseDegrees

    $body = "GPRMC,$timeText,A,$($lat.Value),$($lat.Hemisphere),$($lon.Value),$($lon.Hemisphere),$sogText,$courseText,$dateText,,,A"
    $checksum = Get-NmeaChecksum -Body $body
    return "`$$body*$checksum"
}

$route = @(
    @{ Lat = -4.0426; Lon = 39.6682; Name = "Mombasa anchorage" },
    @{ Lat = -4.0830; Lon = 39.7420; Name = "Kilindini channel" },
    @{ Lat = -4.1550; Lon = 39.8350; Name = "South coast transit" },
    @{ Lat = -4.2450; Lon = 39.9600; Name = "Open water leg" }
)

$udp = [System.Net.Sockets.UdpClient]::new()
$encoding = [System.Text.Encoding]::ASCII
$stepsPerLeg = 45
$tick = 0
$endAt = (Get-Date).AddSeconds($DurationSeconds)

Write-Host "Sending demo RMC feed to udp://$HostName`:$Port"
Write-Host "OpenCPN connection: Network input, UDP, port $Port"
Write-Host "Press Ctrl+C to stop."

try {
    while ((Get-Date) -lt $endAt) {
        $leg = [Math]::Floor($tick / $stepsPerLeg) % ($route.Count - 1)
        $fraction = ($tick % $stepsPerLeg) / [double]$stepsPerLeg

        $from = $route[$leg]
        $to = $route[$leg + 1]
        $lat = $from.Lat + (($to.Lat - $from.Lat) * $fraction)
        $lon = $from.Lon + (($to.Lon - $from.Lon) * $fraction)
        $course = Get-CourseDegrees -FromLat $from.Lat -FromLon $from.Lon -ToLat $to.Lat -ToLon $to.Lon

        $sentence = New-RmcSentence -Latitude $lat -Longitude $lon -SogKnots $SpeedKnots -CourseDegrees $course -Timestamp (Get-Date)
        $bytes = $encoding.GetBytes("$sentence`r`n")
        [void]$udp.Send($bytes, $bytes.Length, $HostName, $Port)

        Write-Host ("{0}  SOG {1:0.0} kn  COG {2:0.0}  Lat {3:0.0000} Lon {4:0.0000}" -f (Get-Date -Format "HH:mm:ss"), $SpeedKnots, $course, $lat, $lon)

        $tick++
        Start-Sleep -Milliseconds $IntervalMs
    }
}
finally {
    $udp.Dispose()
}
