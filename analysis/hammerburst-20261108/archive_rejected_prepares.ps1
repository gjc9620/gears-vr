$ErrorActionPreference='Stop'
$workspaceRoot='C:\Users\11\.codex\worktrees\6dof-single\gearsVr'
$instanceRoot=Join-Path $workspaceRoot 'analysis\instances'
$area=Join-Path $workspaceRoot 'analysis\hammerburst-20261108'
$archiveRoot='J:\gearsVr\analysis\hammerburst-archive-20261110\never-started'
$names=@('hb-cloak-1110-223959','hb-cloak-1110-224419','hb-cloak-1110-224756')
$records=@()
$beforeFree=(Get-PSDrive C).Free
New-Item -ItemType Directory -Path $archiveRoot -Force | Out-Null
foreach($name in $names){
    $source=[IO.Path]::GetFullPath((Join-Path $instanceRoot $name))
    $target=[IO.Path]::GetFullPath((Join-Path $archiveRoot $name))
    $handoff=[IO.Path]::GetFullPath(($source+'-archive-handoff'))
    if($source -ne (Join-Path $instanceRoot $name) -or
       -not $source.StartsWith(($instanceRoot+'\'),[StringComparison]::OrdinalIgnoreCase) -or
       -not $handoff.StartsWith(($instanceRoot+'\'),[StringComparison]::OrdinalIgnoreCase) -or
       -not $target.StartsWith(($archiveRoot+'\'),[StringComparison]::OrdinalIgnoreCase)){
        throw 'Archive path escaped the explicitly named roots'
    }
    if((Test-Path -LiteralPath $target) -or (Test-Path -LiteralPath $handoff)){throw 'Preserve existing archive/handoff; inspect before continuing'}
    $sourceItem=Get-Item -LiteralPath $source
    if($sourceItem.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Source already mapped; do not archive twice'}
    $all=@(Get-ChildItem -LiteralPath $source -Recurse -Force)
    if(@($all|Where-Object {$_.Attributes -band [IO.FileAttributes]::ReparsePoint}).Count){throw 'Nested reparse point refused'}
    foreach($receipt in @('isolated-worker.json','launcher-session.json','isolated-done.json','game-birth.json')){
        if(Test-Path -LiteralPath (Join-Path $source $receipt)){throw 'A runtime receipt exists; only never-started preparations may be archived'}
    }
    $result=Get-Content -LiteralPath (Join-Path (Join-Path $area $name) 'result.json') -Raw|ConvertFrom-Json
    if($result.game_started -or $result.operator_started -or $result.worker_started){throw 'Preparation was used by a runtime'}
    $instance=Get-Content -LiteralPath (Join-Path $source 'instance.json') -Raw|ConvertFrom-Json
    if($instance.root -ne $source -or $instance.name -ne $name){throw 'Prepared instance identity differs'}
    $files=@($all|Where-Object {-not $_.PSIsContainer})
    $verified=@()
    New-Item -ItemType Directory -Path $target | Out-Null
    # Use only native PowerShell filesystem operations throughout this move.
    foreach($entry in $all|Where-Object {$_.PSIsContainer}){
        $relative=$entry.FullName.Substring($source.Length+1)
        New-Item -ItemType Directory -Path (Join-Path $target $relative) -Force | Out-Null
    }
    foreach($file in $files){
        $relative=$file.FullName.Substring($source.Length+1)
        $destination=Join-Path $target $relative
        Copy-Item -LiteralPath $file.FullName -Destination $destination
        $originalHash=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
        $copiedHash=(Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
        if($originalHash -ne $copiedHash -or $file.Length -ne (Get-Item -LiteralPath $destination).Length){throw 'Archive verification failed; originals preserved'}
        $verified+=@{relative=$relative;bytes=$file.Length;sha256=$originalHash.ToLowerInvariant()}
    }
    if(@(Get-ChildItem -LiteralPath $target -File -Recurse -Force).Count -ne $files.Count){throw 'Archive file-set differs'}
    $record=@{name=$name;source=$source;archive=$target;never_started=$true;verified_files=$verified;original_bytes=($files|Measure-Object Length -Sum).Sum;mapping_created=$false;handoff_removed=$false}
    $recordPath=Join-Path $area ($name+'-archive.json')
    $record|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $recordPath -Encoding utf8
    Move-Item -LiteralPath $source -Destination $handoff
    try {
        New-Item -ItemType Junction -Path $source -Target $target | Out-Null
        $mapping=Get-Item -LiteralPath $source
        if(-not ($mapping.Attributes -band [IO.FileAttributes]::ReparsePoint) -or $mapping.Target -ne $target){throw 'Archive mapping differs'}
        foreach($file in $verified){
            if((Get-FileHash -LiteralPath (Join-Path $source $file.relative) -Algorithm SHA256).Hash.ToLowerInvariant() -ne $file.sha256){throw 'Original-path readback differs; handoff preserved'}
        }
        $record.mapping_created=$true
        $record|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $recordPath -Encoding utf8
        # Final destructive target validation: this is the same-volume original
        # handoff under the exact own instance root, never the archive/junction.
        $resolvedHandoff=(Resolve-Path -LiteralPath $handoff).Path
        $handoffItem=Get-Item -LiteralPath $handoff
        if($resolvedHandoff -ne ($source+'-archive-handoff') -or
           -not $resolvedHandoff.StartsWith(($instanceRoot+'\'),[StringComparison]::OrdinalIgnoreCase) -or
           ($handoffItem.Attributes -band [IO.FileAttributes]::ReparsePoint)){
            throw 'Refusing removal of unexpected handoff target'
        }
        Remove-Item -LiteralPath $resolvedHandoff -Recurse
        $record.handoff_removed=$true
        $record|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $recordPath -Encoding utf8
        $records+=@{name=$name;files=$verified.Count;bytes=$record.original_bytes;archive=$target}
        Write-Output ($records[-1]|ConvertTo-Json -Compress)
    } catch {
        # Keep the verified archive and original handoff on any failure.
        throw
    }
}
@{before_free=$beforeFree;after_free=(Get-PSDrive C).Free;instances=$records;game_started=$false;source_or_evidence_discarded=$false}|
    ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $area 'rejected-prepare-archive-20261110.json') -Encoding utf8
