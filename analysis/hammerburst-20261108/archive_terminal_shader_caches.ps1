$ErrorActionPreference='Stop'
$workspaceRoot='C:\Users\11\.codex\worktrees\6dof-single\gearsVr'
$instances=Join-Path $workspaceRoot 'analysis\instances'
$area=Join-Path $workspaceRoot 'analysis\hammerburst-20261108'
$archiveRoot='J:\gearsVr\analysis\hammerburst-archive-20261110\terminal-shader-caches'
$names=@('hb-cloak-1110-221519','hb-cloak-1110-215931','hb-cloak-1110-214502','hb-cloak-1110-225401','hb-cloak-1110-232025','hb-cloak-1110-233210')
$receipts=@();$beforeFree=(Get-PSDrive C).Free
function Assert-IdentityGone($identity){
    if($null -eq $identity -or -not $identity.pid){return}
    $process=Get-Process -Id $identity.pid -ErrorAction SilentlyContinue
    if($null -ne $process){
        if($process.StartTime.ToUniversalTime().ToFileTimeUtc() -eq $identity.creation_time){throw 'Exact owned runtime process is still live; cache archive refused'}
    }
}
foreach($name in $names){
    $instance=[IO.Path]::GetFullPath((Join-Path $instances $name))
    $source=[IO.Path]::GetFullPath((Join-Path $instance 'LocalAppData\Coalition\WarGame\PSOCache'))
    $target=[IO.Path]::GetFullPath((Join-Path (Join-Path $archiveRoot $name) 'PSOCache'))
    $handoff=$source+'-archive-handoff'
    if(-not $source.StartsWith(($instance+'\'),[StringComparison]::OrdinalIgnoreCase) -or
       -not $instance.StartsWith(($instances+'\'),[StringComparison]::OrdinalIgnoreCase) -or
       -not $target.StartsWith(($archiveRoot+'\'),[StringComparison]::OrdinalIgnoreCase)){throw 'Archive path escaped named roots'}
    $wrapper=Get-Content -LiteralPath (Join-Path (Join-Path $area $name) 'result.json') -Raw|ConvertFrom-Json
    $done=Get-Content -LiteralPath (Join-Path $instance 'isolated-done.json') -Raw|ConvertFrom-Json
    if(-not $done.owned_game_absent -or $done.owned_children.remaining.Count -or $done.forced_termination -or $done.worker_error){throw 'Owned cleanup unresolved; preserve cache in place'}
    $worker=Get-Content -LiteralPath (Join-Path $instance 'isolated-worker.json') -Raw|ConvertFrom-Json
    $session=Get-Content -LiteralPath (Join-Path $instance 'launcher-session.json') -Raw|ConvertFrom-Json
    $report=Get-Content -LiteralPath $session.session_report -Raw|ConvertFrom-Json
    foreach($identity in @($wrapper.game_identity,$wrapper.birth_game_identity,$worker.worker_identity,$session.owner_identity,$report.xr_identity)){
        Assert-IdentityGone $identity
    }
    $item=Get-Item -LiteralPath $source
    if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Preserve an existing mapped cache'}
    if((Test-Path -LiteralPath $target) -or (Test-Path -LiteralPath $handoff)){throw 'Archive/handoff already exists; inspect before continuing'}
    $all=@(Get-ChildItem -LiteralPath $source -Recurse -Force)
    if(@($all|Where-Object {$_.Attributes -band [IO.FileAttributes]::ReparsePoint}).Count){throw 'Nested reparse point refused'}
    $files=@($all|Where-Object {-not $_.PSIsContainer});$verified=@()
    New-Item -ItemType Directory -Path $target -Force|Out-Null
    foreach($directory in $all|Where-Object {$_.PSIsContainer}){
        New-Item -ItemType Directory -Path (Join-Path $target $directory.FullName.Substring($source.Length+1)) -Force|Out-Null
    }
    foreach($file in $files){
        $relative=$file.FullName.Substring($source.Length+1);$destination=Join-Path $target $relative
        Copy-Item -LiteralPath $file.FullName -Destination $destination
        $hash=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
        if($hash -ne (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash -or $file.Length -ne (Get-Item -LiteralPath $destination).Length){throw 'Cache archive differs; originals preserved'}
        $verified+=@{relative=$relative;bytes=$file.Length;sha256=$hash.ToLowerInvariant()}
    }
    if(@(Get-ChildItem -LiteralPath $target -File -Recurse -Force).Count -ne $files.Count){throw 'Cache file set differs'}
    $record=@{name=$name;source=$source;archive=$target;identities_checked_absent=$true;verified_files=$verified;bytes=($files|Measure-Object Length -Sum).Sum;mapping_created=$false;handoff_removed=$false;source_logs_media_moved=$false}
    $recordPath=Join-Path $area ($name+'-shader-archive.json')
    $record|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $recordPath -Encoding utf8
    Move-Item -LiteralPath $source -Destination $handoff
    New-Item -ItemType Junction -Path $source -Target $target|Out-Null
    $mapping=Get-Item -LiteralPath $source
    if(-not ($mapping.Attributes -band [IO.FileAttributes]::ReparsePoint) -or $mapping.Target -ne $target){throw 'Mapping differs; verified original handoff preserved'}
    foreach($file in $verified){
        if((Get-FileHash -LiteralPath (Join-Path $source $file.relative) -Algorithm SHA256).Hash.ToLowerInvariant() -ne $file.sha256){throw 'Mapped cache readback differs; handoff preserved'}
    }
    $record.mapping_created=$true
    $record|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $recordPath -Encoding utf8
    $resolved=(Resolve-Path -LiteralPath $handoff).Path;$handoffItem=Get-Item -LiteralPath $handoff
    if($resolved -ne ($source+'-archive-handoff') -or
       -not $resolved.StartsWith(($instance+'\'),[StringComparison]::OrdinalIgnoreCase) -or
       ($handoffItem.Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Refusing unexpected deletion target'}
    Remove-Item -LiteralPath $resolved -Recurse
    $record.handoff_removed=$true
    $record|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $recordPath -Encoding utf8
    $receipts+=@{name=$name;files=$verified.Count;bytes=$record.bytes;archive=$target}
    Write-Output ($receipts[-1]|ConvertTo-Json -Compress)
}
@{before_free=$beforeFree;after_free=(Get-PSDrive C).Free;instances=$receipts;game_started=$false;source_logs_media_moved=$false;cache_contents_preserved=$true}|
    ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $area 'terminal-shader-archive-20261110.json') -Encoding utf8
