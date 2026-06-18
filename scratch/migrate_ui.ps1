$Moves = @(
    @("UI/ContextMenuData", "UI/HUD/ContextMenuData"),
    @("UI/CustomContextMenu", "UI/HUD/CustomContextMenu"),
    @("UI/JupiterHudWidget", "UI/HUD/JupiterHudWidget"),
    @("UI/TacticalWrapperWidget", "UI/HUD/TacticalWrapperWidget"),
    
    @("UI/JupiterUIFactory", "UI/Core/JupiterUIFactory"),
    @("UI/JupiterUISubsystem", "UI/Core/JupiterUISubsystem"),
    @("UI/JupiterUITheme", "UI/Core/JupiterUITheme"),
    
    @("UI/CustomButtonWidget", "UI/Components/CustomButtonWidget"),
    @("UI/CustomSliderWidget", "UI/Components/CustomSliderWidget"),
    @("UI/JupiterToggleSwitch", "UI/Components/JupiterToggleSwitch"),
    
    @("UI/Widgets/JupiterButton", "UI/Components/JupiterButton"),
    @("UI/Widgets/JupiterSlider", "UI/Components/JupiterSlider"),
    @("UI/Widgets/JupiterTacticalSelector", "UI/Components/JupiterTacticalSelector"),
    @("UI/Widgets/JupiterToggle", "UI/Components/JupiterToggle"),
    @("UI/Widgets/JupiterUnitCard", "UI/Components/JupiterUnitCard"),
    @("UI/Widgets/JupiterWidgetBase", "UI/Components/JupiterWidgetBase"),

    @("UI/Editor/JupiterEditorPanel", "UI/Editor/Core/JupiterEditorPanel"),
    @("UI/Editor/JupiterPageBase", "UI/Editor/Core/JupiterPageBase"),
    @("UI/Editor/JupiterUITypes", "UI/Core/JupiterUITypes"),

    @("UI/Editor/Widgets/SidebarButtonWidget", "UI/Editor/Components/SidebarButtonWidget"),
    @("UI/Editor/Widgets/Patrol/PatrolDetailWidget", "UI/Editor/Components/Patrol/PatrolDetailWidget"),
    @("UI/Editor/Widgets/Patrol/PatrolEntryWidget", "UI/Editor/Components/Patrol/PatrolEntryWidget"),
    @("UI/Editor/Widgets/Patrol/PatrolRoutePreview", "UI/Editor/Components/Patrol/PatrolRoutePreview"),
    @("UI/Editor/Widgets/Spawn/PresetEntryWidget", "UI/Editor/Components/Spawn/PresetEntryWidget"),
    @("UI/Editor/Widgets/Spawn/PropsEntryWidget", "UI/Editor/Components/Spawn/PropsEntryWidget"),
    @("UI/Editor/Widgets/Spawn/UnitsEntryWidget", "UI/Editor/Components/Spawn/UnitsEntryWidget"),
    @("UI/Editor/Widgets/Spawn/UnitSpawnAxisWidget", "UI/Editor/Components/Spawn/UnitSpawnAxisWidget"),
    @("UI/Editor/Widgets/Spawn/UnitSpawnCountWidget", "UI/Editor/Components/Spawn/UnitSpawnCountWidget"),

    @("UI/Behaviors/BehaviorButtonWidget", "UI/Tactical/Behaviors/BehaviorButtonWidget"),
    @("UI/Behaviors/SelectBehaviorWidget", "UI/Tactical/Behaviors/SelectBehaviorWidget"),

    @("UI/Formations/FormationButtonWidget", "UI/Tactical/Formations/FormationButtonWidget"),
    @("UI/Formations/FormationSelectorWidget", "UI/Tactical/Formations/FormationSelectorWidget")
)

$PublicDir = "c:\Apps\Unreal\Projects\DevPlugin\Plugins\JupiterPlugin\Source\JupiterPlugin\Public"
$PrivateDir = "c:\Apps\Unreal\Projects\DevPlugin\Plugins\JupiterPlugin\Source\JupiterPlugin\Private"
$SourceDir = "c:\Apps\Unreal\Projects\DevPlugin\Plugins\JupiterPlugin\Source\JupiterPlugin"

foreach ($Move in $Moves) {
    $OldPath = $Move[0]
    $NewPath = $Move[1]

    # Handle Public (.h)
    $OldH = Join-Path $PublicDir "$OldPath.h"
    $NewH = Join-Path $PublicDir "$NewPath.h"
    
    if (Test-Path $OldH) {
        $NewHDir = Split-Path $NewH
        if (-not (Test-Path $NewHDir)) { New-Item -ItemType Directory -Force -Path $NewHDir | Out-Null }
        Move-Item -Path $OldH -Destination $NewH -Force
        Write-Host "Moved $OldH to $NewH"
    }

    # Handle Private (.cpp)
    $OldCpp = Join-Path $PrivateDir "$OldPath.cpp"
    $NewCpp = Join-Path $PrivateDir "$NewPath.cpp"
    
    if (Test-Path $OldCpp) {
        $NewCppDir = Split-Path $NewCpp
        if (-not (Test-Path $NewCppDir)) { New-Item -ItemType Directory -Force -Path $NewCppDir | Out-Null }
        Move-Item -Path $OldCpp -Destination $NewCpp -Force
        Write-Host "Moved $OldCpp to $NewCpp"
    }
}

# Recursively update requires/includes
$AllFiles = Get-ChildItem -Path $SourceDir -Recurse -Include *.cpp,*.h

foreach ($File in $AllFiles) {
    $Content = [System.IO.File]::ReadAllText($File.FullName)
    $OriginalContent = $Content
    $Modified = $false

    foreach ($Move in $Moves) {
        $OldPath = $Move[0]
        $OldBasename = Split-Path $OldPath -Leaf
        $NewPath = $Move[1]
        
        # Replace occurrences of #include "UI/OldPath.h" with #include "UI/NewPath.h"
        # We need to catch forward slashes
        $OldIncludeFull = "`"$OldPath.h`""
        $NewIncludeFull = "`"$NewPath.h`""
        
        if ($Content -match [regex]::Escape($OldIncludeFull)) {
            $Content = $Content -replace [regex]::Escape($OldIncludeFull), $NewIncludeFull
            $Modified = $true
        }

        # Also handle potential direct includes if people didn't use the full UI/ path initially
        # E.g. #include "JupiterUIFactory.h"  -> will be updated to #include "UI/Core/JupiterUIFactory.h"
        # ONLY IF the previous include was just the basename.
        $OldIncludeShort = "`"$OldBasename.h`""
        if ($Content -match [regex]::Escape($OldIncludeShort)) {
            $Content = $Content -replace [regex]::Escape($OldIncludeShort), $NewIncludeFull
            $Modified = $true
        }
    }

    if ($Modified -and ($Content -ne $OriginalContent)) {
        [System.IO.File]::WriteAllText($File.FullName, $Content)
        Write-Host "Updated includes in $($File.Name)"
    }
}
