========================================================================
                      LEVEL SELECTION PLUGIN
========================================================================

Author: Bixboy
Supported Platforms: Windows, Mac, Linux
Supported Engine Versions: 5.3, 5.4, 5.5, 5.6, 5.7

------------------------------------------------------------------------
1. INTRODUCTION
------------------------------------------------------------------------
The Level Selection Plugin is a premium Editor Extension designed to
radically speed up your workflow. It automatically discovers all maps 
in your project and organizes them into a clean, easy-to-use dropdown 
menu directly integrated into the Unreal Engine main toolbar. 

Say goodbye to constantly searching through the Content Browser for 
your levels!

------------------------------------------------------------------------
2. KEY FEATURES
------------------------------------------------------------------------
* Automatic Map Discovery: Uses background caching via the Asset 
  Registry to find every map in your project instantly. No hitches!
* Custom Categories: Organize your levels into custom, named categories
  (e.g., "Main Menu", "Cinematics", "Test Maps").
* Favorites System (NEW!): Pin your most-used levels to the very top
  of the menu for 1-click access.
* Thumbnail Tooltips (NEW!): Hover over any level name to instantly 
  see a 256x256 thumbnail preview of the map before opening it.
* Set Default Editor Map: Go to Options on any level to instantly
  set it as the Editor Startup Map.
* Content Browser Integration: "Locate in Content Browser" feature
  allows you to jump directly to the asset folder.

------------------------------------------------------------------------
3. INSTALLATION
------------------------------------------------------------------------
1. Extract the `LevelSelection` folder into your Unreal Engine project's 
   `Plugins` directory.
   Example: [YourProjectName]/Plugins/LevelSelection/
   (If the "Plugins" folder doesn't exist, simply create it.)

2. Launch your Unreal Engine project.
3. If prompted, click "Yes" to rebuild the plugin for your engine version.
4. Go to Edit > Plugins, and ensure "Level Selection" is Enabled.

------------------------------------------------------------------------
4. HOW TO USE
------------------------------------------------------------------------
* The Plugin adds a "Level Selection" dropdown menu directly in the main 
  toolbar (next to the Help menu).
* Click it to see all your project maps.
* Hover over any map and open the "Options" submenu to:
   - Add/Remove from Favorites
   - Create a New Category and move the map
   - Copy the Level Name
   - Set as Default Editor Level
   - Locate the map in the Content Browser

------------------------------------------------------------------------
5. TROUBLESHOOTING
------------------------------------------------------------------------
* Q: A map I just created doesn't appear in the list!
  A: Make sure you have saved the map at least once. The plugin relies 
     on the Asset Registry, which updates when assets are saved to disk.

* Q: The plugin doesn't show up in the toolbar!
  A: Verify the plugin is enabled in Edit > Plugins. Also ensure you are 
     using a supported Engine version.

========================================================================
Enjoy faster iteration times and a cleaner workflow!
========================================================================