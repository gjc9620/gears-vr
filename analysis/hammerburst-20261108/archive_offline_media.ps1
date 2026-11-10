$ErrorActionPreference='Stop'
$area='C:\Users\11\.codex\worktrees\6dof-single\gearsVr\analysis\hammerburst-20261108'
$archiveRoot='J:\gearsVr\analysis\hammerburst-archive-20261110\offline-media'
$names=@('runtime-52960-offline','runtime-56252-offline','runtime-53484-offline')
$summary=Join-Path $area 'offline-media-archive-20261111.json'
if(Test-Path -LiteralPath $summary){throw 'Preserve existing summary'}
$beforeFree=(Get-PSDrive C).Free;$records=@()
foreach($name in $names){
    $source=[IO.Path]::GetFullPath((Join-Path $area $name))
    $target=[IO.Path]::GetFullPath((Join-Path $archiveRoot $name));$handoff=$source+'-archive-handoff'
    if($source -ne (Join-Path $area $name) -or
       -not $source.StartsWith(($area+'\'),[StringComparison]::OrdinalIgnoreCase) -or
       -not $target.StartsWith(($archiveRoot+'\'),[StringComparison]::OrdinalIgnoreCase)){throw 'Path escaped explicit roots'}
    if((Test-Path -LiteralPath $target) -or (Test-Path -LiteralPath $handoff)){throw 'Preserve existing target/handoff'}
    if((Get-Item -LiteralPath $source).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Source already mapped'}
    $all=@(Get-ChildItem -LiteralPath $source -Recurse -Force)
    if(@($all|Where-Object {$_.Attributes -band [IO.FileAttributes]::ReparsePoint}).Count){throw 'Nested mapping refused'}
    $files=@($all|Where-Object {-not $_.PSIsContainer})
    if(@($files|Where-Object {$_.Extension -notin @('.json','.log','.jpg','.jsonl','.mp4','')}).Count){throw 'Only reviewed offline data types allowed; do not move source or binaries'}
    if(@($files|Where-Object {$_.LastWriteTime -gt (Get-Date).AddMinutes(-30)}).Count){throw 'Recently changing offline data refused'}
    New-Item -ItemType Directory -Path $target -Force|Out-Null
    foreach($directory in $all|Where-Object {$_.PSIsContainer}){
        New-Item -ItemType Directory -Path (Join-Path $target $directory.FullName.Substring($source.Length+1)) -Force|Out-Null
    }
    $verified=@()
    foreach($file in $files){
        $relative=$file.FullName.Substring($source.Length+1);$destination=Join-Path $target $relative
        Copy-Item -LiteralPath $file.FullName -Destination $destination
        $hash=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
        if($hash -ne (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -or $file.Length -ne (Get-Item -LiteralPath $destination).Length){throw 'Copy differs; original preserved'}
        $verified+=@{relative=$relative;bytes=$file.Length;sha256=$hash.ToLowerInvariant()}
    }
    if(@(Get-ChildItem -LiteralPath $target -File -Recurse -Force).Count -ne $files.Count){throw 'File-set differs'}
    $record=@{name=$name;source=$source;archive=$target;files=$verified;bytes=($files|Measure-Object Length -Sum).Sum;mapping_created=$false;handoff_removed=$false;game_started=$false;fit_accepted=$false}
    $recordPath=Join-Path $area ($name+'-archive.json')
    if(Test-Path -LiteralPath $recordPath){throw 'Preserve previous archive proof'}
    $record|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $recordPath -Encoding utf8
    Move-Item -LiteralPath $source -Destination $handoff
    New-Item -ItemType Junction -Path $source -Target $target|Out-Null
    $mapping=Get-Item -LiteralPath $source
    if(-not ($mapping.Attributes -band [IO.FileAttributes]::ReparsePoint) -or $mapping.Target -ne $target){throw 'Mapping differs; handoff preserved'}
    foreach($file in $verified){
        if((Get-FileHash -LiteralPath (Join-Path $source $file.relative) -Algorithm SHA256).Hash.ToLowerInvariant() -ne $file.sha256){throw 'Mapped readback differs; handoff preserved'}
    }
    $record.mapping_created=$true
    $record|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $recordPath -Encoding utf8
    $resolved=(Resolve-Path -LiteralPath $handoff).Path;$handoffItem=Get-Item -LiteralPath $handoff
    if($resolved -ne ($source+'-archive-handoff') -or
       -not $resolved.StartsWith(($area+'\'),[StringComparison]::OrdinalIgnoreCase) -or
       ($handoffItem.Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Refuse unexpected removal target'}
    Remove-Item -LiteralPath $resolved -Recurse
    $record.handoff_removed=$true
    $record|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $recordPath -Encoding utf8
    $records+=@{name=$name;files=$verified.Count;bytes=$record.bytes;archive=$target}
    Write-Output ($records[-1]|ConvertTo-Json -Compress)
}
@{before_free=$beforeFree;after_free=(Get-PSDrive C).Free;archives=$records;source_or_binary_moved=$false;all_original_data_preserved=$true}|
    ConvertTo-Json -Depth 5|Set-Content -LiteralPath $summary -Encoding utf8
