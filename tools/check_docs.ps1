[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$scriptDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $scriptDirectory ".."))
$repositoryPrefix = $repositoryRoot.TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar
$strictUtf8 = [System.Text.UTF8Encoding]::new($false, $true)
$issues = New-Object 'System.Collections.Generic.List[string]'
$headingCache = @{}
$titleCache = @{}

function Add-CheckIssue
{
    param(
        [string]$Code,
        [string]$File,
        [int]$Line,
        [string]$Message
    )

    if ($Line -gt 0)
    {
        $issues.Add(("[{0}] {1}:{2} {3}" -f $Code, $File, $Line, $Message))
    }
    else
    {
        $issues.Add(("[{0}] {1} {2}" -f $Code, $File, $Message))
    }
}

function ConvertTo-MarkdownAnchor
{
    param([string]$Heading)

    $anchor = $Heading.ToLowerInvariant()
    $anchor = [regex]::Replace($anchor, '<[^>]+>', '')
    $anchor = [regex]::Replace($anchor, '[^\p{L}\p{M}\p{Nd}\s_-]', '')
    $anchor = [regex]::Replace($anchor.Trim(), '\s+', '-')
    return $anchor
}

function Get-MarkdownHeadings
{
    param([string]$Path)

    if ($headingCache.ContainsKey($Path))
    {
        return $headingCache[$Path]
    }

    $anchors = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    $counts = @{}
    $lines = [System.IO.File]::ReadAllLines($Path, $strictUtf8)

    foreach ($line in $lines)
    {
        if ($line -notmatch '^#{1,6}\s+(.+?)\s*#*\s*$')
        {
            continue
        }

        $baseAnchor = ConvertTo-MarkdownAnchor $matches[1]
        if ([string]::IsNullOrWhiteSpace($baseAnchor))
        {
            continue
        }

        if ($counts.ContainsKey($baseAnchor))
        {
            $counts[$baseAnchor]++
            $anchor = "{0}-{1}" -f $baseAnchor, $counts[$baseAnchor]
        }
        else
        {
            $counts[$baseAnchor] = 0
            $anchor = $baseAnchor
        }

        [void]$anchors.Add($anchor)
    }

    $headingCache[$Path] = $anchors
    return $anchors
}

function Get-MarkdownTitle
{
    param([string]$Path)

    if ($titleCache.ContainsKey($Path))
    {
        return $titleCache[$Path]
    }

    $title = ''
    foreach ($line in [System.IO.File]::ReadAllLines($Path, $strictUtf8))
    {
        if ($line -match '^#\s+(.+?)\s*$')
        {
            $title = $matches[1]
            break
        }
    }

    $titleCache[$Path] = $title
    return $title
}

function Test-DocumentCategory
{
    param(
        [string]$RelativePath,
        [string]$Category
    )

    $escapedCategory = [regex]::Escape($Category)
    return $RelativePath -match ("(^|[\\/]){0}([\\/]|$)" -f $escapedCategory)
}

function Get-LineNavigationKind
{
    param(
        [string]$Line,
        [string]$SectionKind
    )

    if ($Line -match '^[-*]\s*(?:相关)?\s*(应用(?:文档)?|设计(?:文档)?|教材|基础教材|源码|代码)[：:]')
    {
        $prefix = $matches[1]
        if ($prefix -match '应用') { return 'application' }
        if ($prefix -match '设计') { return 'design' }
        if ($prefix -match '教材') { return 'tutorial' }
        return 'source'
    }

    return $SectionKind
}

function Test-LinkTitle
{
    param(
        [string]$Label,
        [string]$Title,
        [string]$File,
        [int]$Line
    )

    if ([string]::IsNullOrWhiteSpace($Title))
    {
        Add-CheckIssue 'TITLE' $File $Line '目标 Markdown 缺少一级标题。'
        return
    }

    $checks = @(
        @{ Label = '设计'; Title = '设计' },
        @{ Label = '使用'; Title = '使用' },
        @{ Label = '移植'; Title = '移植' },
        @{ Label = '教程'; Title = '教程' },
        @{ Label = '总纲'; Title = '总纲' }
    )

    foreach ($check in $checks)
    {
        if (($Label -match $check.Label) -and ($Title -notmatch $check.Title))
        {
            Add-CheckIssue 'SEMANTIC' $File $Line ("链接名称「{0}」与目标标题「{1}」的文档类型不一致。" -f $Label, $Title)
        }
    }

    if (($Label -match '应用(?:文档)?') -and ($Title -notmatch '应用|使用'))
    {
        Add-CheckIssue 'SEMANTIC' $File $Line ("应用导航「{0}」指向标题「{1}」。" -f $Label, $Title)
    }
}

Push-Location $repositoryRoot
try
{
    $markdownFiles = @(& git ls-files --cached --others --exclude-standard -- '*.md' |
        Sort-Object -Unique |
        Where-Object { Test-Path -LiteralPath $_ -PathType Leaf })

    if ($LASTEXITCODE -ne 0)
    {
        throw '无法从 Git 获取 Markdown 文件清单。'
    }

    $linkCount = 0
    $imageCount = 0

    foreach ($relativeFile in $markdownFiles)
    {
        $fullFile = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot $relativeFile))
        try
        {
            $content = [System.IO.File]::ReadAllText($fullFile, $strictUtf8)
        }
        catch
        {
            Add-CheckIssue 'UTF8' $relativeFile 0 '文件不是严格 UTF-8 编码。'
            continue
        }

        $lines = $content -split "\r?\n"
        $inNavigation = $false
        $navigationKind = ''
        $hasNavigation = $false
        $navigationKindsFound = New-Object 'System.Collections.Generic.HashSet[string]'
        $isDesignDocument = (Test-DocumentCategory $relativeFile 'design') -and ($relativeFile -notmatch '_INDEX\.md$')
        $isApplicationDocument = (Test-DocumentCategory $relativeFile 'application') -and ($relativeFile -notmatch '_INDEX\.md$')
        $isTutorialDocument = Test-DocumentCategory $relativeFile 'tutorial'
        $isIndex = $relativeFile -match '_INDEX\.md$'

        for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++)
        {
            $line = $lines[$lineIndex]
            $lineNumber = $lineIndex + 1

            if ($line -match '^## .*关联导航\s*$')
            {
                $inNavigation = $true
                $hasNavigation = $true
                $navigationKind = ''
            }
            elseif ($inNavigation -and ($line -match '^## '))
            {
                $inNavigation = $false
                $navigationKind = ''
            }
            elseif ($inNavigation -and ($line -match '^###\s*(.+)$'))
            {
                $heading = $matches[1]
                if ($heading -match '应用') { $navigationKind = 'application' }
                elseif ($heading -match '设计') { $navigationKind = 'design' }
                elseif ($heading -match '教材|基础') { $navigationKind = 'tutorial' }
                elseif ($heading -match '源码|代码') { $navigationKind = 'source' }
                else { $navigationKind = '' }
            }

            $lineKind = Get-LineNavigationKind $line $navigationKind
            $matchesOnLine = [regex]::Matches($line, '(!?)\[([^\]]*)\]\(([^)]+)\)')
            foreach ($linkMatch in $matchesOnLine)
            {
                $linkCount++
                $isImage = $linkMatch.Groups[1].Value -eq '!'
                if ($isImage) { $imageCount++ }

                $label = $linkMatch.Groups[2].Value
                $rawTarget = $linkMatch.Groups[3].Value.Trim()
                if ($rawTarget.StartsWith('<') -and $rawTarget.EndsWith('>'))
                {
                    $rawTarget = $rawTarget.Substring(1, $rawTarget.Length - 2)
                }

                if ($rawTarget -match '^(https?|mailto|ftp):')
                {
                    continue
                }

                $targetParts = $rawTarget -split '#', 2
                $targetPath = [System.Uri]::UnescapeDataString($targetParts[0])
                $targetAnchor = if ($targetParts.Count -gt 1) { [System.Uri]::UnescapeDataString($targetParts[1]) } else { '' }

                if ([string]::IsNullOrWhiteSpace($targetPath))
                {
                    $resolvedTarget = $fullFile
                }
                else
                {
                    $resolvedTarget = [System.IO.Path]::GetFullPath((Join-Path (Split-Path -Parent $fullFile) $targetPath))
                }

                if (($resolvedTarget -ne $repositoryRoot) -and
                    (-not $resolvedTarget.StartsWith($repositoryPrefix, [System.StringComparison]::OrdinalIgnoreCase)))
                {
                    Add-CheckIssue 'OUTSIDE' $relativeFile $lineNumber ("链接越出仓库：{0}" -f $rawTarget)
                    continue
                }

                if (-not (Test-Path -LiteralPath $resolvedTarget))
                {
                    Add-CheckIssue 'MISSING' $relativeFile $lineNumber ("目标不存在：{0}" -f $rawTarget)
                    continue
                }

                if (Test-Path -LiteralPath $resolvedTarget -PathType Container)
                {
                    Add-CheckIssue 'DIRECTORY' $relativeFile $lineNumber ("导航必须指向具体文件：{0}" -f $rawTarget)
                    continue
                }

                $targetExtension = [System.IO.Path]::GetExtension($resolvedTarget)
                $targetRelative = $resolvedTarget.Substring($repositoryPrefix.Length)

                if (-not [string]::IsNullOrWhiteSpace($targetAnchor))
                {
                    if ($targetExtension -ne '.md')
                    {
                        Add-CheckIssue 'ANCHOR' $relativeFile $lineNumber ("非 Markdown 文件不能使用标题锚点：{0}" -f $rawTarget)
                    }
                    else
                    {
                        $anchors = Get-MarkdownHeadings $resolvedTarget
                        if (-not $anchors.Contains($targetAnchor))
                        {
                            Add-CheckIssue 'ANCHOR' $relativeFile $lineNumber ("标题锚点不存在：{0}" -f $rawTarget)
                        }
                    }
                }

                $expectedKind = ''
                if ($inNavigation -and ($lineKind -in @('application', 'design', 'tutorial', 'source')))
                {
                    $expectedKind = $lineKind
                    [void]$navigationKindsFound.Add($lineKind)
                }
                elseif ($relativeFile -eq 'docs\application\APPLICATION_INDEX.md')
                {
                    $expectedKind = 'application'
                }
                elseif ($relativeFile -eq 'docs\design\DESIGN_INDEX.md')
                {
                    $expectedKind = 'design'
                }
                elseif ($relativeFile -eq 'docs\tutorial\TUTORIAL_INDEX.md')
                {
                    $expectedKind = 'tutorial'
                }

                if ($expectedKind -in @('application', 'design', 'tutorial'))
                {
                    if ($targetExtension -ne '.md')
                    {
                        Add-CheckIssue 'DOC_TYPE' $relativeFile $lineNumber ("{0}导航必须指向 Markdown 文档：{1}" -f $expectedKind, $rawTarget)
                    }
                    elseif (($expectedKind -eq 'design') -and ($targetRelative -eq 'docs\engineering_design.md'))
                    {
                        # 工程总设计位于 docs 根目录，是设计导航的唯一根级入口。
                    }
                    elseif (-not (Test-DocumentCategory $targetRelative $expectedKind))
                    {
                        Add-CheckIssue 'CATEGORY' $relativeFile $lineNumber ("{0}导航指向了错误分类：{1}" -f $expectedKind, $rawTarget)
                    }
                }

                if (($targetExtension -eq '.md') -and (($inNavigation -and ($lineKind -ne 'source')) -or $isIndex))
                {
                    Test-LinkTitle $label (Get-MarkdownTitle $resolvedTarget) $relativeFile $lineNumber
                }
            }

            if ($line -match '^\s*\[[^\]]+\]:\s*(\S+)')
            {
                $linkCount++
                $referenceTarget = $matches[1]
                if ($referenceTarget -notmatch '^(https?|mailto|ftp):')
                {
                    $referencePath = ($referenceTarget -split '#', 2)[0]
                    $resolvedReference = [System.IO.Path]::GetFullPath((Join-Path (Split-Path -Parent $fullFile) $referencePath))
                    if (-not (Test-Path -LiteralPath $resolvedReference -PathType Leaf))
                    {
                        Add-CheckIssue 'MISSING' $relativeFile $lineNumber ("引用式链接目标不存在：{0}" -f $referenceTarget)
                    }
                }
            }
        }

        if (($isDesignDocument -or $isApplicationDocument) -and (-not $hasNavigation))
        {
            Add-CheckIssue 'NAVIGATION' $relativeFile 0 '设计或应用文档缺少「关联导航」。'
        }

        if ($isDesignDocument -and $hasNavigation -and (-not $navigationKindsFound.Contains('application')))
        {
            Add-CheckIssue 'NAVIGATION' $relativeFile 0 '设计文档的关联导航缺少应用文档。'
        }

        if ($isApplicationDocument -and $hasNavigation)
        {
            if (-not $navigationKindsFound.Contains('source'))
            {
                Add-CheckIssue 'NAVIGATION' $relativeFile 0 '应用文档的关联导航缺少源码。'
            }
            if (-not $navigationKindsFound.Contains('design'))
            {
                Add-CheckIssue 'NAVIGATION' $relativeFile 0 '应用文档的关联导航缺少设计文档。'
            }
        }

        if ($isTutorialDocument -and $hasNavigation)
        {
            Add-CheckIssue 'NAVIGATION' $relativeFile 0 '教材应保持独立，不应包含工程关联导航。'
        }
    }

    Write-Host ("Markdown files : {0}" -f $markdownFiles.Count)
    Write-Host ("Links          : {0}" -f $linkCount)
    Write-Host ("Images         : {0}" -f $imageCount)
    Write-Host ("Issues         : {0}" -f $issues.Count)

    if ($issues.Count -gt 0)
    {
        foreach ($issue in $issues)
        {
            Write-Host $issue -ForegroundColor Red
        }
        exit 1
    }

    Write-Host 'Documentation navigation check passed.' -ForegroundColor Green
    exit 0
}
finally
{
    Pop-Location
}
