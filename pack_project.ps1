$OutputFile = "project_context_export.md"

# 初始化输出文件 (UTF-8 编码)
Set-Content -Path $OutputFile -Value "# ECG Monitor Project Context" -Encoding utf8
Add-Content -Path $OutputFile -Value ""
Add-Content -Path $OutputFile -Value "Generated automatically by PowerShell script."
Add-Content -Path $OutputFile -Value ""

# 需要排除的目录和文件后缀
$ExcludeDirs = @(".git", ".vs", "out", "build")
$ExcludeExts = @(".md", ".sln", ".vcxproj", ".user", ".json", ".ico", ".exe", ".obj", ".ilk", ".pdb", ".ps1")

# 递归遍历项目目录下的所有文件
Get-ChildItem -Recurse -File | ForEach-Object {
    $file = $_
    $relativePath = $file.FullName.Substring((Get-Location).Path.Length + 1)
    $pathParts = $relativePath -split '[/\\]'
    
    # 检查是否在排除目录中
    $skip = $false
    foreach ($part in $pathParts) {
        if ($ExcludeDirs -contains $part) {
            $skip = $true
            break
        }
    }
    
    # 检查文件后缀或名称是否需要排除
    if ($skip -eq $false -and 
        $ExcludeExts -notcontains $file.Extension.ToLower() -and 
        $file.Name -ne $OutputFile) {
        
        $relPath = $relativePath -replace '\\', '/'
        Write-Host "Packing: $relPath"

        # 写入 Markdown 标题
        Add-Content -Path $OutputFile -Value "## File: $relPath"
        Add-Content -Path $OutputFile -Value ""

        # 确定代码块高亮语言
        $lang = ""
        $ext = $file.Extension.ToLower()
        if ($ext -eq ".cpp" -or $ext -eq ".h" -or $ext -eq ".rc") {
            $lang = "cpp"
        } elseif ($ext -eq ".ui" -or $ext -eq ".qrc") {
            $lang = "xml"
        } elseif ($ext -eq ".css") {
            $lang = "css"
        } elseif ($file.Name -eq "CMakeLists.txt" -or $file.Name -eq "qt.cmake") {
            $lang = "cmake"
        } elseif ($ext -eq ".txt") {
            $lang = "txt"
        }

        # 写入代码块开头
        Add-Content -Path $OutputFile -Value ('```' + $lang)

        # 使用更稳健的 .NET 方式读取文件内容（自动适配编码）
        try {
            $content = [System.IO.File]::ReadAllText($file.FullName)
            [System.IO.File]::AppendAllText((Resolve-Path $OutputFile),$content + "`n", [System.Text.Encoding]::UTF8)
        } catch {
            Add-Content -Path $OutputFile -Value "// Error reading file: $_"
        }

        # 写入代码块结尾
        Add-Content -Path $OutputFile -Value '```'
        Add-Content -Path $OutputFile -Value ""
    }
}

Write-Host "=========================================="
Write-Host "打包完成！已生成汇总文件: $OutputFile"
Write-Host "=========================================="