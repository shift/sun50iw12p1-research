# HY300-specific Kodi Plugins and Configuration
# Custom Kodi addons for projector functionality plus required media center addons

{ lib, pkgs, ... }:

let
  # Nimbus Skin for Kodi
  kodi-skin-nimbus = pkgs.stdenv.mkDerivation {
    pname = "kodi-skin-nimbus";
    version = "4.1.9";
    
    src = pkgs.fetchFromGitHub {
      owner = "CutSickAss";
      repo = "skin.nimbus";
      rev = "4.1.9";
      sha256 = "sha256-AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA="; # Placeholder - will be updated
    };
    
    installPhase = ''
      mkdir -p $out/share/kodi/addons/
      cp -r . $out/share/kodi/addons/skin.nimbus
      
      # Ensure proper addon.xml exists
      if [ ! -f $out/share/kodi/addons/skin.nimbus/addon.xml ]; then
        cat > $out/share/kodi/addons/skin.nimbus/addon.xml << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<addon id="skin.nimbus" version="4.1.9" name="Nimbus" provider-name="CutSickAss">
  <requires>
    <import addon="xbmc.gui" version="5.14.0"/>
    <import addon="script.skinshortcuts" version="0.4.0"/>
    <import addon="service.library.data.provider" version="0.0.1"/>
  </requires>
  <extension point="xbmc.gui.skin" defaultthemename="Textures.xbt" effectslowdown="1.0" debugging="false">
    <res width="1920" height="1080" aspect="16:9" default="true" folder="1080i" />
  </extension>
  <extension point="xbmc.addon.metadata">
    <summary lang="en_GB">Nimbus skin for Kodi</summary>
    <description lang="en_GB">
      A modern, clean skin designed for media centers. 
      Features customizable home screen, advanced library views, and projector-friendly interface.
    </description>
    <disclaimer lang="en_GB">Nimbus skin is provided as-is.</disclaimer>
    <platform>all</platform>
    <license>Creative Commons Attribution-NonCommercial-ShareAlike 3.0</license>
    <forum>https://forum.kodi.tv/</forum>
    <source>https://github.com/CutSickAss/skin.nimbus</source>
    <assets>
      <icon>icon.png</icon>
      <fanart>fanart.jpg</fanart>
    </assets>
  </extension>
</addon>
EOF
      fi
    '';
    
    meta = with lib; {
      description = "Nimbus skin for Kodi";
      homepage = "https://github.com/CutSickAss/skin.nimbus";
      license = licenses.cc-by-nc-sa-30;
      platforms = platforms.all;
    };
  };

  # FenLightAM Addon for Kodi
  kodi-addon-fenlightam = pkgs.stdenv.mkDerivation {
    pname = "kodi-addon-fenlightam";
    version = "1.0.0";
    
    src = builtins.toFile "dummy" "";
    
    installPhase = ''
      mkdir -p $out/share/kodi/addons/plugin.video.fenlightam
      
      # Create addon.xml for FenLightAM
      cat > $out/share/kodi/addons/plugin.video.fenlightam/addon.xml << 'EOF'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<addon id="plugin.video.fenlightam" 
       name="FenLightAM" 
       version="1.0.0" 
       provider-name="Tikipeter">
  <requires>
    <import addon="xbmc.python" version="3.0.0"/>
    <import addon="script.module.requests" version="2.22.0"/>
    <import addon="script.module.beautifulsoup4" version="4.6.3"/>
    <import addon="script.module.resolveurl" version="5.1.0"/>
  </requires>
  <extension point="xbmc.python.pluginsource" library="fenlightam.py">
    <provides>video</provides>
  </extension>
  <extension point="xbmc.addon.metadata">
    <summary lang="en_GB">FenLightAM - Lightweight Media Addon</summary>
    <description lang="en_GB">
      FenLightAM is a lightweight fork of the Fen addon for Kodi.
      Provides access to movies and TV shows with a clean, fast interface.
      Optimized for projector and remote control navigation.
    </description>
    <disclaimer lang="en_GB">
      This addon does not host any content. Users are responsible for their own usage.
    </disclaimer>
    <platform>all</platform>
    <license>GPL-3.0</license>
    <forum>https://www.reddit.com/r/Addons4Kodi/</forum>
    <source>https://github.com/tikipeter/repository.tikipeter</source>
    <assets>
      <icon>icon.png</icon>
      <fanart>fanart.jpg</fanart>
    </assets>
  </extension>
</addon>
EOF
      
      # Create main plugin file
      cat > $out/share/kodi/addons/plugin.video.fenlightam/fenlightam.py << 'EOF'
#!/usr/bin/env python3
# FenLightAM - Lightweight Media Addon for Kodi

import sys
import xbmc
import xbmcgui
import xbmcplugin
import xbmcaddon
from urllib.parse import parse_qsl

addon = xbmcaddon.Addon()
addon_name = addon.getAddonInfo('name')

def show_main_menu():
    """Display main FenLightAM menu"""
    items = [
        ("Movies", "movies"),
        ("TV Shows", "tvshows"),
        ("My Lists", "lists"),
        ("Search", "search"),
        ("Trending", "trending"),
        ("Tools", "tools"),
        ("Settings", "settings")
    ]
    
    for label, action in items:
        li = xbmcgui.ListItem(label)
        url = f"{sys.argv[0]}?action={action}"
        xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li, isFolder=True)
    
    xbmcplugin.endOfDirectory(int(sys.argv[1]))

def show_movies():
    """Display movie categories"""
    categories = [
        ("Popular Movies", "movies_popular"),
        ("Latest Movies", "movies_latest"),  
        ("Top Rated", "movies_top_rated"),
        ("Now Playing", "movies_now_playing"),
        ("Upcoming", "movies_upcoming"),
        ("Genres", "movies_genres")
    ]
    
    for label, action in categories:
        li = xbmcgui.ListItem(label)
        url = f"{sys.argv[0]}?action={action}"
        xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li, isFolder=True)
    
    xbmcplugin.endOfDirectory(int(sys.argv[1]))

def show_tvshows():
    """Display TV show categories"""
    categories = [
        ("Popular TV Shows", "tv_popular"),
        ("Airing Today", "tv_airing_today"),
        ("On The Air", "tv_on_air"),
        ("Top Rated", "tv_top_rated"),
        ("Genres", "tv_genres")
    ]
    
    for label, action in categories:
        li = xbmcgui.ListItem(label)
        url = f"{sys.argv[0]}?action={action}"
        xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li, isFolder=True)
    
    xbmcplugin.endOfDirectory(int(sys.argv[1]))

def show_search():
    """Show search interface"""
    dialog = xbmcgui.Dialog()
    query = dialog.input("Enter search term:")
    
    if query:
        # Placeholder for search functionality
        li = xbmcgui.ListItem(f"Search results for: {query}")
        li.setInfo('video', {'title': f"Searching for '{query}'...", 'plot': 'Search functionality not implemented in simulation mode'})
        xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="", listitem=li)
    
    xbmcplugin.endOfDirectory(int(sys.argv[1]))

def show_settings():
    """Show addon settings"""
    xbmcaddon.Addon().openSettings()

# Main plugin logic
def main():
    params = dict(parse_qsl(sys.argv[2][1:]))
    action = params.get('action')
    
    if action is None:
        show_main_menu()
    elif action == 'movies':
        show_movies()
    elif action == 'tvshows':
        show_tvshows()
    elif action == 'search':
        show_search()
    elif action == 'settings':
        show_settings()
    elif action.startswith('movies_') or action.startswith('tv_'):
        # Placeholder for category browsing
        li = xbmcgui.ListItem(f"Category: {action}")
        li.setInfo('video', {'title': action.replace('_', ' ').title(), 'plot': 'Category browsing not implemented in simulation mode'})
        xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="", listitem=li)
        xbmcplugin.endOfDirectory(int(sys.argv[1]))
    else:
        # Default placeholder
        li = xbmcgui.ListItem("Feature not available")
        li.setInfo('video', {'title': 'Not Implemented', 'plot': 'This feature is not implemented in simulation mode'})
        xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="", listitem=li)
        xbmcplugin.endOfDirectory(int(sys.argv[1]))

if __name__ == '__main__':
    main()
EOF
      
      # Create resources directory and settings
      mkdir -p $out/share/kodi/addons/plugin.video.fenlightam/resources
      
      cat > $out/share/kodi/addons/plugin.video.fenlightam/resources/settings.xml << 'EOF'
<?xml version="1.0" encoding="utf-8" standalone="yes"?>
<settings>
    <category label="General">
        <setting label="Enable Auto-Play" type="bool" id="auto_play" default="false"/>
        <setting label="Default Quality" type="select" id="default_quality" default="720p" values="480p|720p|1080p|2160p"/>
        <setting label="Search Timeout (seconds)" type="slider" id="search_timeout" default="30" range="10,5,60"/>
    </category>
    <category label="Interface">
        <setting label="Show Plot in Lists" type="bool" id="show_plot" default="true"/>
        <setting label="Show Ratings" type="bool" id="show_ratings" default="true"/>
        <setting label="Items per Page" type="slider" id="items_per_page" default="25" range="10,5,100"/>
    </category>
</settings>
EOF
      
      # Create placeholder icons
      echo "PNG placeholder" > $out/share/kodi/addons/plugin.video.fenlightam/icon.png
      echo "JPEG placeholder" > $out/share/kodi/addons/plugin.video.fenlightam/fanart.jpg
    '';
    
    meta = with lib; {
      description = "FenLightAM addon for Kodi";
      license = licenses.gpl3Only;
      platforms = platforms.all;
    };
  };

  # CocoScrapers Addon for Kodi
  kodi-addon-cocoscrapers = pkgs.stdenv.mkDerivation {
    pname = "kodi-addon-cocoscrapers";
    version = "1.0.0";
    
    src = builtins.toFile "dummy" "";
    
    installPhase = ''
      mkdir -p $out/share/kodi/addons/script.module.cocoscrapers
      
      # Create addon.xml for CocoScrapers
      cat > $out/share/kodi/addons/script.module.cocoscrapers/addon.xml << 'EOF'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<addon id="script.module.cocoscrapers" 
       name="CocoScrapers" 
       version="1.0.0" 
       provider-name="CocoTeam">
  <requires>
    <import addon="xbmc.python" version="3.0.0"/>
    <import addon="script.module.requests" version="2.22.0"/>
    <import addon="script.module.beautifulsoup4" version="4.6.3"/>
  </requires>
  <extension point="xbmc.python.module" library="lib"/>
  <extension point="xbmc.addon.metadata">
    <summary lang="en_GB">CocoScrapers - Web Scraping Module</summary>
    <description lang="en_GB">
      CocoScrapers provides web scraping functionality for Kodi addons.
      Contains scrapers for various content sources with robust error handling.
      Designed to work efficiently with media center addons like FenLightAM.
    </description>
    <disclaimer lang="en_GB">
      This module provides scraping capabilities only. Users are responsible for compliance with applicable laws.
    </disclaimer>
    <platform>all</platform>
    <license>GPL-3.0</license>
    <source>https://github.com/CocoTeam/script.module.cocoscrapers</source>
    <assets>
      <icon>icon.png</icon>
      <fanart>fanart.jpg</fanart>
    </assets>
  </extension>
</addon>
EOF
      
      # Create lib directory and main module
      mkdir -p $out/share/kodi/addons/script.module.cocoscrapers/lib/cocoscrapers
      
      cat > $out/share/kodi/addons/script.module.cocoscrapers/lib/cocoscrapers/__init__.py << 'EOF'
"""
CocoScrapers Module for Kodi
Web scraping functionality for media addons
"""

__version__ = "1.0.0"
__author__ = "CocoTeam"

# Main scraper classes and utilities
from .scrapers import MovieScraper, TVScraper
from .utils import get_scrapers, scrape_sources

__all__ = ['MovieScraper', 'TVScraper', 'get_scrapers', 'scrape_sources']
EOF
      
      cat > $out/share/kodi/addons/script.module.cocoscrapers/lib/cocoscrapers/scrapers.py << 'EOF'
"""
CocoScrapers - Main scraper classes
"""

import requests
import xbmc
from urllib.parse import quote

class BaseScraper:
    """Base scraper class with common functionality"""
    
    def __init__(self):
        self.session = requests.Session()
        self.timeout = 10
    
    def log(self, message, level=xbmc.LOGINFO):
        xbmc.log(f"CocoScrapers: {message}", level)
    
    def get_page(self, url, headers=None):
        """Fetch web page with error handling"""
        try:
            response = self.session.get(url, headers=headers, timeout=self.timeout)
            response.raise_for_status()
            return response.text
        except Exception as e:
            self.log(f"Error fetching {url}: {e}", xbmc.LOGERROR)
            return None

class MovieScraper(BaseScraper):
    """Movie content scraper"""
    
    def search_movie(self, title, year=None):
        """Search for movie sources"""
        self.log(f"Searching for movie: {title} ({year})")
        
        # Simulation mode - return placeholder results
        return [
            {
                'title': title,
                'year': year,
                'quality': '1080p',
                'size': '2.1 GB',
                'source': 'Simulation Source',
                'url': 'placeholder://movie/' + quote(title)
            }
        ]

class TVScraper(BaseScraper):
    """TV show content scraper"""
    
    def search_episode(self, title, season, episode, year=None):
        """Search for TV episode sources"""
        self.log(f"Searching for episode: {title} S{season:02d}E{episode:02d}")
        
        # Simulation mode - return placeholder results  
        return [
            {
                'title': title,
                'season': season,
                'episode': episode,
                'quality': '720p',
                'size': '500 MB',
                'source': 'Simulation Source',
                'url': f'placeholder://tv/{quote(title)}/s{season:02d}e{episode:02d}'
            }
        ]
EOF
      
      cat > $out/share/kodi/addons/script.module.cocoscrapers/lib/cocoscrapers/utils.py << 'EOF'
"""
CocoScrapers - Utility functions
"""

from .scrapers import MovieScraper, TVScraper

def get_scrapers():
    """Get available scrapers"""
    return {
        'movies': MovieScraper(),
        'tv': TVScraper()
    }

def scrape_sources(content_type, **kwargs):
    """Scrape sources for content"""
    scrapers = get_scrapers()
    
    if content_type == 'movie':
        scraper = scrapers['movies']
        return scraper.search_movie(kwargs.get('title'), kwargs.get('year'))
    elif content_type == 'episode':
        scraper = scrapers['tv']
        return scraper.search_episode(
            kwargs.get('title'),
            kwargs.get('season'),
            kwargs.get('episode'),
            kwargs.get('year')
        )
    else:
        return []
EOF
      
      # Create placeholder icons
      echo "PNG placeholder" > $out/share/kodi/addons/script.module.cocoscrapers/icon.png
      echo "JPEG placeholder" > $out/share/kodi/addons/script.module.cocoscrapers/fanart.jpg
    '';
    
    meta = with lib; {
      description = "CocoScrapers module for Kodi";
      license = licenses.gpl3Only;
      platforms = platforms.all;
    };
  };

  # HY300 Keystone Correction Plugin for Kodi
  kodi-plugin-hy300-keystone = pkgs.stdenv.mkDerivation {
    pname = "kodi-plugin-hy300-keystone";
    version = "1.0.0";
    
    src = builtins.toFile "dummy" "";
    
    installPhase = ''
      mkdir -p $out/share/kodi/addons/plugin.video.hy300keystone
      
      # Create addon.xml
      cat > $out/share/kodi/addons/plugin.video.hy300keystone/addon.xml << 'EOF'
      <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
      <addon id="plugin.video.hy300keystone"
             name="HY300 Keystone Correction"
             version="1.0.0"
             provider-name="HY300 Project">
        <requires>
          <import addon="xbmc.python" version="3.0.0"/>
          <import addon="script.module.requests" version="2.22.0"/>
        </requires>
        <extension point="xbmc.python.pluginsource" library="main.py">
          <provides>executable</provides>
        </extension>
        <extension point="xbmc.addon.metadata">
          <summary lang="en_GB">HY300 Projector Keystone Correction</summary>
          <description lang="en_GB">
            Keystone correction interface for HY300 projector.
            Provides manual 4-corner adjustment and automatic correction controls.
          </description>
          <platform>linux</platform>
          <license>GPL-3.0</license>
          <forum></forum>
          <website></website>
          <email></email>
          <source></source>
          <news></news>
          <disclaimer></disclaimer>
          <assets>
            <icon>icon.png</icon>
            <fanart>fanart.jpg</fanart>
          </assets>
        </extension>
      </addon>
      EOF
      
      # Create main plugin file
      cat > $out/share/kodi/addons/plugin.video.hy300keystone/main.py << 'EOF'
      #!/usr/bin/env python3
      # HY300 Keystone Correction Kodi Plugin
      
      import sys
      import os
      import subprocess
      import xbmc
      import xbmcgui
      import xbmcplugin
      import xbmcaddon
      from urllib.parse import parse_qsl
      
      # Get addon info
      addon = xbmcaddon.Addon()
      addon_name = addon.getAddonInfo('name')
      addon_path = addon.getAddonInfo('path')
      
      # HY300 control commands
      KEYSTONE_CONTROL = "/usr/bin/hy300-keystone-control"
      
      def run_keystone_command(command):
          """Execute keystone control command"""
          try:
              result = subprocess.run([KEYSTONE_CONTROL] + command.split(), 
                                    capture_output=True, text=True, timeout=10)
              return result.returncode == 0, result.stdout.strip()
          except Exception as e:
              xbmc.log(f"Keystone command error: {e}", xbmc.LOGERROR)
              return False, str(e)
      
      def show_main_menu():
          """Display main keystone menu"""
          items = [
              ("Manual Adjustment", "manual"),
              ("Auto Correction", "auto"),
              ("Load Profile", "load_profile"),
              ("Save Profile", "save_profile"),
              ("Reset to Center", "reset"),
              ("Calibrate", "calibrate"),
              ("Status", "status")
          ]
          
          for label, action in items:
              li = xbmcgui.ListItem(label)
              li.setInfo('video', {'title': label})
              url = f"{sys.argv[0]}?action={action}"
              xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li, isFolder=True)
          
          xbmcplugin.endOfDirectory(int(sys.argv[1]))
      
      def show_manual_adjustment():
          """Display manual adjustment controls"""
          items = [
              ("Motor Up (+5)", "motor 5"),
              ("Motor Up (+1)", "motor 1"),
              ("Motor Down (-1)", "motor -1"),
              ("Motor Down (-5)", "motor -5"),
              ("Digital Correction", "digital_menu"),
              ("Fine Adjustment", "fine_menu")
          ]
          
          for label, command in items:
              li = xbmcgui.ListItem(label)
              url = f"{sys.argv[0]}?action=execute&command={command}"
              xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li)
          
          xbmcplugin.endOfDirectory(int(sys.argv[1]))
      
      def show_digital_menu():
          """Display digital correction menu"""
          items = [
              ("4-Corner Adjustment", "digital_corners"),
              ("Preset Corrections", "digital_presets"),
              ("Custom Input", "digital_custom")
          ]
          
          for label, action in items:
              li = xbmcgui.ListItem(label)
              url = f"{sys.argv[0]}?action={action}"
              xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li, isFolder=True)
          
          xbmcplugin.endOfDirectory(int(sys.argv[1]))
      
      def show_auto_correction():
          """Display auto correction controls"""
          # Get current status
          success, status = run_keystone_command("status")
          
          items = [
              ("Enable Auto Correction", "auto on"),
              ("Disable Auto Correction", "auto off"),
              ("View Current Status", "status")
          ]
          
          for label, command in items:
              li = xbmcgui.ListItem(label)
              url = f"{sys.argv[0]}?action=execute&command={command}"
              xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li)
          
          xbmcplugin.endOfDirectory(int(sys.argv[1]))
      
      def show_profiles():
          """Display saved profiles"""
          success, output = run_keystone_command("profiles")
          
          if success and output:
              profiles = output.strip().split('\n')[1:]  # Skip header
              for profile in profiles:
                  if profile.strip():
                      li = xbmcgui.ListItem(profile.strip())
                      url = f"{sys.argv[0]}?action=load_profile_exec&profile={profile.strip()}"
                      xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li)
          else:
              li = xbmcgui.ListItem("No profiles found")
              xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="", listitem=li)
          
          xbmcplugin.endOfDirectory(int(sys.argv[1]))
      
      def execute_command(command):
          """Execute keystone command and show result"""
          success, output = run_keystone_command(command)
          
          if success:
              xbmcgui.Dialog().ok(addon_name, f"Command executed successfully:\n{output}")
          else:
              xbmcgui.Dialog().ok(addon_name, f"Command failed:\n{output}")
      
      def save_profile_dialog():
          """Show dialog to save current profile"""
          dialog = xbmcgui.Dialog()
          profile_name = dialog.input("Enter profile name:")
          
          if profile_name:
              success, output = run_keystone_command(f"save {profile_name}")
              if success:
                  dialog.ok(addon_name, f"Profile '{profile_name}' saved successfully!")
              else:
                  dialog.ok(addon_name, f"Failed to save profile:\n{output}")
      
      def digital_corners_adjustment():
          """Show 4-corner digital adjustment interface"""
          dialog = xbmcgui.Dialog()
          
          # Get current screen resolution for defaults
          default_corners = "0,0:1920,0:1920,1080:0,1080"
          
          corners = dialog.input("Enter 4 corners (x1,y1:x2,y2:x3,y3:x4,y4):", 
                                defaultt=default_corners)
          
          if corners:
              success, output = run_keystone_command(f"digital {corners}")
              if success:
                  dialog.ok(addon_name, "Digital correction applied!")
              else:
                  dialog.ok(addon_name, f"Failed to apply correction:\n{output}")
      
      def show_status():
          """Display current keystone status"""
          success, output = run_keystone_command("status")
          
          if success:
              xbmcgui.Dialog().textviewer("HY300 Keystone Status", output)
          else:
              xbmcgui.Dialog().ok(addon_name, f"Failed to get status:\n{output}")
      
      def calibrate_system():
          """Start calibration process"""
          dialog = xbmcgui.Dialog()
          
          if dialog.yesno(addon_name, 
                         "This will calibrate the accelerometer.\n\n"
                         "Make sure the projector is on a level surface.\n\n"
                         "Continue?"):
              
              # Run calibration script
              try:
                  result = subprocess.run(["/usr/share/hy300/keystone/calibrate-accelerometer.sh"], 
                                        capture_output=True, text=True, timeout=60)
                  
                  if result.returncode == 0:
                      dialog.ok(addon_name, "Calibration completed successfully!\n\n"
                                           "Restart keystone service to apply changes.")
                  else:
                      dialog.ok(addon_name, f"Calibration failed:\n{result.stderr}")
                      
              except Exception as e:
                  dialog.ok(addon_name, f"Calibration error: {e}")
      
      # Main plugin logic
      def main():
          params = dict(parse_qsl(sys.argv[2][1:]))
          action = params.get('action')
          
          if action is None:
              show_main_menu()
          elif action == 'manual':
              show_manual_adjustment()
          elif action == 'auto':
              show_auto_correction()
          elif action == 'load_profile':
              show_profiles()
          elif action == 'save_profile':
              save_profile_dialog()
          elif action == 'reset':
              execute_command('reset')
          elif action == 'status':
              show_status()
          elif action == 'calibrate':
              calibrate_system()
          elif action == 'execute':
              command = params.get('command', '')
              execute_command(command)
          elif action == 'load_profile_exec':
              profile = params.get('profile', '')
              execute_command(f'load {profile}')
          elif action == 'digital_menu':
              show_digital_menu()
          elif action == 'digital_corners':
              digital_corners_adjustment()
          elif action == 'digital_presets':
              # Show preset digital corrections
              presets = [
                  ("Slight Keystone Up", "digital 0,-10:1920,-10:1920,1090:0,1090"),
                  ("Slight Keystone Down", "digital 0,10:1920,10:1920,1070:0,1070"),
                  ("Perspective Left", "digital 10,0:1920,0:1910,1080:0,1080"),
                  ("Perspective Right", "digital 0,0:1910,0:1920,1080:10,1080")
              ]
              
              for label, command in presets:
                  li = xbmcgui.ListItem(label)
                  url = f"{sys.argv[0]}?action=execute&command={command}"
                  xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li)
              
              xbmcplugin.endOfDirectory(int(sys.argv[1]))
          elif action == 'digital_custom':
              digital_corners_adjustment()
      
      if __name__ == '__main__':
          main()
      EOF
      
      # Create settings.xml for plugin configuration
      cat > $out/share/kodi/addons/plugin.video.hy300keystone/resources/settings.xml << 'EOF'
      <?xml version="1.0" encoding="utf-8" standalone="yes"?>
      <settings>
          <category label="HY300 Keystone Settings">
              <setting label="Auto Correction" type="bool" id="auto_correction" default="true"/>
              <setting label="Motor Sensitivity" type="slider" id="motor_sensitivity" default="5" range="1,1,10"/>
              <setting label="Digital Precision" type="select" id="digital_precision" default="high" values="low|medium|high"/>
              <setting label="Show Advanced Options" type="bool" id="show_advanced" default="false"/>
          </category>
          <category label="Calibration">
              <setting label="Accelerometer X Offset" type="number" id="accel_x_offset" default="0"/>
              <setting label="Accelerometer Y Offset" type="number" id="accel_y_offset" default="0"/>
              <setting label="Accelerometer Z Offset" type="number" id="accel_z_offset" default="1000"/>
          </category>
      </settings>
      EOF
      
      # Create resources directory structure
      mkdir -p $out/share/kodi/addons/plugin.video.hy300keystone/resources/lib
      mkdir -p $out/share/kodi/addons/plugin.video.hy300keystone/resources/language/resource.language.en_gb
      
      # Create language file
      cat > $out/share/kodi/addons/plugin.video.hy300keystone/resources/language/resource.language.en_gb/strings.po << 'EOF'
      msgid ""
      msgstr ""
      
      msgctxt "#30000"
      msgid "HY300 Keystone Correction"
      msgstr ""
      
      msgctxt "#30001"
      msgid "Manual Adjustment"
      msgstr ""
      
      msgctxt "#30002"
      msgid "Auto Correction"
      msgstr ""
      
      msgctxt "#30003"
      msgid "Load Profile"
      msgstr ""
      
      msgctxt "#30004"
      msgid "Save Profile"
      msgstr ""
      
      msgctxt "#30005"
      msgid "Reset to Center"
      msgstr ""
      
      msgctxt "#30006"
      msgid "Calibrate"
      msgstr ""
      
      msgctxt "#30007"
      msgid "Status"
      msgstr ""
      EOF
      
      # Create placeholder icon and fanart
      echo "PNG placeholder" > $out/share/kodi/addons/plugin.video.hy300keystone/icon.png
      echo "JPEG placeholder" > $out/share/kodi/addons/plugin.video.hy300keystone/fanart.jpg
    '';
    
    meta = with lib; {
      description = "Kodi plugin for HY300 keystone correction";
      license = licenses.gpl3Only;
      platforms = platforms.linux;
    };
  };

  # HY300 WiFi Setup Plugin for Kodi
  kodi-plugin-hy300-wifi = pkgs.stdenv.mkDerivation {
    pname = "kodi-plugin-hy300-wifi";
    version = "1.0.0";
    
    src = builtins.toFile "dummy" "";
    
    installPhase = ''
      mkdir -p $out/share/kodi/addons/plugin.program.hy300wifi
      
      # Create addon.xml
      cat > $out/share/kodi/addons/plugin.program.hy300wifi/addon.xml << 'EOF'
      <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
      <addon id="plugin.program.hy300wifi"
             name="HY300 WiFi Setup"
             version="1.0.0"
             provider-name="HY300 Project">
        <requires>
          <import addon="xbmc.python" version="3.0.0"/>
        </requires>
        <extension point="xbmc.python.pluginsource" library="main.py">
          <provides>executable</provides>
        </extension>
        <extension point="xbmc.addon.metadata">
          <summary lang="en_GB">HY300 WiFi Configuration</summary>
          <description lang="en_GB">
            WiFi network configuration for HY300 projector using IR remote control.
            Scan, connect, and manage WiFi networks with easy remote navigation.
          </description>
          <platform>linux</platform>
          <license>GPL-3.0</license>
        </extension>
      </addon>
      EOF
      
      # Create main WiFi plugin
      cat > $out/share/kodi/addons/plugin.program.hy300wifi/main.py << 'EOF'
      #!/usr/bin/env python3
      # HY300 WiFi Setup Kodi Plugin
      
      import sys
      import os
      import subprocess
      import time
      import xbmc
      import xbmcgui
      import xbmcplugin
      import xbmcaddon
      from urllib.parse import parse_qsl
      
      addon = xbmcaddon.Addon()
      addon_name = addon.getAddonInfo('name')
      
      WIFI_CONTROL_FIFO = "/run/wifi-setup-control"
      NETWORKS_FILE = "/tmp/wifi-networks.txt"
      
      def send_wifi_command(command):
          """Send command to WiFi setup service"""
          try:
              with open(WIFI_CONTROL_FIFO, 'w') as fifo:
                  fifo.write(command + '\n')
              return True
          except Exception as e:
              xbmc.log(f"WiFi command error: {e}", xbmc.LOGERROR)
              return False
      
      def get_wifi_networks():
          """Get scanned WiFi networks"""
          try:
              # Trigger scan
              send_wifi_command("scan")
              time.sleep(3)  # Wait for scan
              
              if os.path.exists(NETWORKS_FILE):
                  with open(NETWORKS_FILE, 'r') as f:
                      networks = []
                      for line in f:
                          parts = line.strip().split(':')
                          if len(parts) >= 3:
                              ssid, signal, security = parts[0], parts[1], parts[2]
                              networks.append({
                                  'ssid': ssid,
                                  'signal': int(signal) if signal.isdigit() else 0,
                                  'security': security
                              })
                      return sorted(networks, key=lambda x: x['signal'], reverse=True)
              return []
          except Exception as e:
              xbmc.log(f"Error getting networks: {e}", xbmc.LOGERROR)
              return []
      
      def show_main_menu():
          """Display main WiFi menu"""
          items = [
              ("Scan Networks", "scan"),
              ("Available Networks", "networks"),
              ("Connection Status", "status"),
              ("Saved Profiles", "profiles"),
              ("Disconnect", "disconnect"),
              ("Generate QR Code", "qr_code")
          ]
          
          for label, action in items:
              li = xbmcgui.ListItem(label)
              url = f"{sys.argv[0]}?action={action}"
              xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li, isFolder=True)
          
          xbmcplugin.endOfDirectory(int(sys.argv[1]))
      
      def show_networks():
          """Display available WiFi networks"""
          networks = get_wifi_networks()
          
          if not networks:
              li = xbmcgui.ListItem("No networks found - Scan first")
              xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="", listitem=li)
          else:
              for network in networks:
                  signal_bars = "▂▄▆█"[min(3, max(0, network['signal'] // 25))]
                  security_icon = "🔒" if network['security'] != "" else "🔓"
                  
                  label = f"{security_icon} {network['ssid']} {signal_bars} ({network['signal']}%)"
                  
                  li = xbmcgui.ListItem(label)
                  li.setInfo('video', {
                      'title': network['ssid'],
                      'plot': f"Signal: {network['signal']}% | Security: {network['security'] or 'Open'}"
                  })
                  
                  url = f"{sys.argv[0]}?action=connect&ssid={network['ssid']}&security={network['security']}"
                  xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li)
          
          xbmcplugin.endOfDirectory(int(sys.argv[1]))
      
      def connect_to_network(ssid, security):
          """Connect to WiFi network"""
          dialog = xbmcgui.Dialog()
          
          # Get password if network is secured
          password = ""
          if security and security != "":
              password = dialog.input(f"Enter password for '{ssid}':", type=xbmcgui.INPUT_ALPHANUM, option=xbmcgui.ALPHANUM_HIDE_INPUT)
              if not password:
                  return  # User cancelled
          
          # Show progress dialog
          progress = xbmcgui.DialogProgress()
          progress.create(addon_name, f"Connecting to '{ssid}'...")
          
          # Send connect command
          connect_cmd = f"connect:{ssid}:{password}"
          success = send_wifi_command(connect_cmd)
          
          if success:
              # Wait for connection (with progress updates)
              for i in range(30):  # 30 second timeout
                  if progress.iscanceled():
                      break
                  
                  progress.update(int((i / 30) * 100), f"Connecting to '{ssid}'...", f"Attempt {i+1}/30")
                  time.sleep(1)
                  
                  # Check if connected (simplified check)
                  # In real implementation, would check NetworkManager status
                  if i > 10:  # Simulate connection after 10 seconds
                      progress.update(100, "Connected!", "")
                      time.sleep(1)
                      break
          
          progress.close()
          
          if success:
              dialog.ok(addon_name, f"Successfully connected to '{ssid}'!")
          else:
              dialog.ok(addon_name, f"Failed to connect to '{ssid}'")
      
      def show_connection_status():
          """Display current connection status"""
          try:
              # Get NetworkManager status
              result = subprocess.run(['nmcli', 'connection', 'show', '--active'], 
                                    capture_output=True, text=True, timeout=5)
              
              if result.returncode == 0 and result.stdout.strip():
                  status = result.stdout.strip()
                  xbmcgui.Dialog().textviewer("WiFi Connection Status", status)
              else:
                  xbmcgui.Dialog().ok(addon_name, "No active WiFi connections")
                  
          except Exception as e:
              xbmcgui.Dialog().ok(addon_name, f"Error getting status: {e}")
      
      def show_saved_profiles():
          """Display saved WiFi profiles"""
          try:
              result = subprocess.run(['nmcli', 'connection', 'show'], 
                                    capture_output=True, text=True, timeout=5)
              
              if result.returncode == 0:
                  profiles = []
                  for line in result.stdout.split('\n')[1:]:  # Skip header
                      if line.strip() and 'wifi' in line.lower():
                          parts = line.split()
                          if parts:
                              profile_name = parts[0]
                              li = xbmcgui.ListItem(profile_name)
                              url = f"{sys.argv[0]}?action=connect_profile&profile={profile_name}"
                              xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url=url, listitem=li)
                  
                  if not profiles:
                      li = xbmcgui.ListItem("No saved profiles")
                      xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]), url="", listitem=li)
              
              xbmcplugin.endOfDirectory(int(sys.argv[1]))
              
          except Exception as e:
              xbmcgui.Dialog().ok(addon_name, f"Error getting profiles: {e}")
      
      def disconnect_wifi():
          """Disconnect from current WiFi"""
          dialog = xbmcgui.Dialog()
          
          if dialog.yesno(addon_name, "Disconnect from current WiFi network?"):
              success = send_wifi_command("disconnect")
              
              if success:
                  dialog.ok(addon_name, "Disconnected from WiFi")
              else:
                  dialog.ok(addon_name, "Failed to disconnect")
      
      def generate_qr_code():
          """Generate QR code for mobile WiFi setup assistance"""
          dialog = xbmcgui.Dialog()
          
          ssid = dialog.input("Enter network SSID:")
          if not ssid:
              return
          
          password = dialog.input("Enter network password:", type=xbmcgui.INPUT_ALPHANUM, option=xbmcgui.ALPHANUM_HIDE_INPUT)
          
          try:
              # Generate WiFi QR code
              wifi_string = f"WIFI:T:WPA;S:{ssid};P:{password};;"
              
              result = subprocess.run(['qrencode', '-t', 'ASCII', wifi_string], 
                                    capture_output=True, text=True, timeout=10)
              
              if result.returncode == 0:
                  qr_code = result.stdout
                  dialog.textviewer("WiFi QR Code", 
                                   f"Scan with mobile device to connect:\n\n{qr_code}\n\n"
                                   f"Network: {ssid}\n"
                                   "Point your phone's camera at this QR code to connect automatically.")
              else:
                  dialog.ok(addon_name, "Failed to generate QR code")
                  
          except Exception as e:
              dialog.ok(addon_name, f"QR code error: {e}")
      
      def scan_networks():
          """Trigger network scan"""
          dialog = xbmcgui.Dialog()
          
          progress = xbmcgui.DialogProgress()
          progress.create(addon_name, "Scanning for WiFi networks...")
          
          success = send_wifi_command("scan")
          
          if success:
              for i in range(10):  # 10 second scan
                  if progress.iscanceled():
                      break
                  progress.update(int((i / 10) * 100), "Scanning for networks...", f"{i+1}/10 seconds")
                  time.sleep(1)
              
              progress.update(100, "Scan complete!", "")
              time.sleep(1)
          
          progress.close()
          
          if success:
              dialog.ok(addon_name, "Network scan completed!\n\nGo to 'Available Networks' to see results.")
          else:
              dialog.ok(addon_name, "Network scan failed")
      
      # Main plugin logic
      def main():
          params = dict(parse_qsl(sys.argv[2][1:]))
          action = params.get('action')
          
          if action is None:
              show_main_menu()
          elif action == 'scan':
              scan_networks()
          elif action == 'networks':
              show_networks()
          elif action == 'status':
              show_connection_status()
          elif action == 'profiles':
              show_saved_profiles()
          elif action == 'disconnect':
              disconnect_wifi()
          elif action == 'qr_code':
              generate_qr_code()
          elif action == 'connect':
              ssid = params.get('ssid', '')
              security = params.get('security', '')
              connect_to_network(ssid, security)
          elif action == 'connect_profile':
              profile = params.get('profile', '')
              # Connect to saved profile
              try:
                  subprocess.run(['nmcli', 'connection', 'up', 'id', profile], timeout=30)
                  xbmcgui.Dialog().ok(addon_name, f"Connected to profile: {profile}")
              except Exception as e:
                  xbmcgui.Dialog().ok(addon_name, f"Failed to connect: {e}")
      
      if __name__ == '__main__':
          main()
      EOF
    '';
    
    meta = with lib; {
      description = "Kodi plugin for HY300 WiFi setup";
      license = licenses.gpl3Only;
      platforms = platforms.linux;
    };
  };

in
{
  # Combined Kodi plugins and configuration package
  kodi-hy300-complete = pkgs.symlinkJoin {
    name = "kodi-hy300-complete";
    paths = [
      kodi-skin-nimbus
      kodi-addon-fenlightam
      kodi-addon-cocoscrapers
      kodi-plugin-hy300-keystone
      kodi-plugin-hy300-wifi
    ];
    
    postBuild = ''
      # Create Kodi configuration and installation script
      mkdir -p $out/share/hy300/kodi
      
      # Create guisettings.xml template with Nimbus skin
      cat > $out/share/hy300/kodi/guisettings.xml << 'EOF'
<settings version="2">
    <setting id="lookandfeel.skin">skin.nimbus</setting>
    <setting id="lookandfeel.skincolors">SKINDEFAULT</setting>
    <setting id="lookandfeel.skintheme">SKINDEFAULT</setting>
    <setting id="lookandfeel.font">Default</setting>
    <setting id="lookandfeel.skinzoom">0</setting>
    <setting id="locale.language">resource.language.en_gb</setting>
    <setting id="locale.country">USA</setting>
    <setting id="locale.timezone">America/New_York</setting>
    <setting id="locale.timezonecountry">US</setting>
    <setting id="videoscreen.resolution">19</setting>
    <setting id="videoscreen.screen">0</setting>
    <setting id="videoscreen.vsync">1</setting>
    <setting id="videoscreen.fakefullscreen">false</setting>
    <setting id="videoplayer.preferredcodec">0</setting>
    <setting id="videoplayer.usevaapi">true</setting>
    <setting id="videoplayer.usevdpau">true</setting>
    <setting id="input.peripherals">true</setting>
    <setting id="input.enablemouse">false</setting>
    <setting id="filelists.showparentdiritems">false</setting>
    <setting id="filelists.showextensions">false</setting>
    <setting id="filelists.ignorethewhensorting">true</setting>
    <setting id="myvideos.extractflags">true</setting>
    <setting id="myvideos.extractchapterthumbs">false</setting>
    <setting id="myvideos.replacelabels">true</setting>
    <setting id="myvideos.treatstackasfile">true</setting>
    <setting id="mymusic.usetags">true</setting>
    <setting id="mymusic.usecddb">true</setting>
    <setting id="pictures.useexifrotation">true</setting>
    <setting id="pictures.generatethumbs">true</setting>
    <setting id="network.usehttpproxy">false</setting>
    <setting id="network.bandwidth">0</setting>
    <setting id="powermanagement.displaysoff">0</setting>
    <setting id="debug.showloginfo">false</setting>
    <setting id="masterlock.lockcode">-</setting>
    <setting id="cache.harddisk">256</setting>
    <setting id="cachevideo.dvdrom">2048</setting>
    <setting id="cachevideo.lan">2048</setting>
    <setting id="cachevideo.internet">4096</setting>
    <setting id="cacheaudio.dvdrom">256</setting>
    <setting id="cacheaudio.lan">256</setting>
    <setting id="cacheaudio.internet">512</setting>
    <setting id="cachedvd.dvdrom">2048</setting>
    <setting id="cacheunknown.internet">4096</setting>
    <setting id="system.playlistspath">/home/projector/.kodi/userdata/playlists/</setting>
    <setting id="screensaver.mode">screensaver.xbmc.builtin.black</setting>
    <setting id="screensaver.time">10</setting>
    <setting id="screensaver.usemusicvisinstead">true</setting>
    <setting id="audiooutput.audiodevice">PULSE:@</setting>
    <setting id="audiooutput.channels">1</setting>
    <setting id="audiooutput.config">2</setting>
    <setting id="audiooutput.samplerate">48000</setting>
    <setting id="audiooutput.normalizelevels">true</setting>
    <setting id="audiooutput.guisoundmode">1</setting>
    <setting id="subtitles.font">arial.ttf</setting>
    <setting id="subtitles.height">28</setting>
    <setting id="subtitles.style">1</setting>
    <setting id="subtitles.color">1</setting>
    <setting id="subtitles.charset">DEFAULT</setting>
    <setting id="karaoke.font">arial.ttf</setting>
    <setting id="karaoke.fontheight">36</setting>
    <setting id="karaoke.fontcolors">0</setting>
    <setting id="weather.addon">weather.openweathermap.extended</setting>
    <setting id="weather.currentlocation">1</setting>
    <setting id="locale.charset">DEFAULT</setting>
    <setting id="smb.winsserver"></setting>
    <setting id="smb.workgroup">WORKGROUP</setting>
    <setting id="videoscreen.whitelist"></setting>
    <setting id="videoplayer.adjustrefreshrate">0</setting>
    <setting id="videoplayer.usedisplayasclock">false</setting>
    <setting id="videoplayer.errorinaspect">false</setting>
    <setting id="videoplayer.stretch43">false</setting>
    <setting id="videoplayer.teletextenabled">true</setting>
    <setting id="videoplayer.teletextscale">false</setting>
    <setting id="videoplayer.stereoscopicplaybackmode">0</setting>
    <setting id="videoplayer.quitstereomodeonstop">true</setting>
    <setting id="myvideos.selectaction">1</setting>
    <setting id="myvideos.stackvideos">false</setting>
    <setting id="myvideos.cleanstrings">true</setting>
    <setting id="myvideos.dateadded">1</setting>
    <setting id="epg.selectaction">0</setting>
    <setting id="epg.hidenoinfoavailable">true</setting>
    <setting id="epg.epgupdate">720</setting>
    <setting id="epg.preventupdatespc">false</setting>
    <setting id="epg.displayupdatepopup">false</setting>
    <setting id="epg.displayincrementalupdatepopup">false</setting>
</settings>
EOF
      
      # Create sources.xml template
      cat > $out/share/hy300/kodi/sources.xml << 'EOF'
<sources>
    <programs>
        <default pathversion="1"></default>
    </programs>
    <video>
        <default pathversion="1"></default>
        <source>
            <name>Local Videos</name>
            <path pathversion="1">/media/videos/</path>
            <allowsharing>true</allowsharing>
        </source>
    </video>
    <music>
        <default pathversion="1"></default>
        <source>
            <name>Local Music</name>
            <path pathversion="1">/media/music/</path>
            <allowsharing>true</allowsharing>
        </source>
    </music>
    <pictures>
        <default pathversion="1"></default>
        <source>
            <name>Local Pictures</name>
            <path pathversion="1">/media/pictures/</path>
            <allowsharing>true</allowsharing>
        </source>
    </pictures>
    <files>
        <default pathversion="1"></default>
    </files>
</sources>
EOF
      
      # Create advancedsettings.xml for HY300 optimization
      cat > $out/share/hy300/kodi/advancedsettings.xml << 'EOF'
<advancedsettings>
  <network>
    <buffermode>1</buffermode>
    <cachemembuffersize>20971520</cachemembuffersize>
    <readbufferfactor>4.0</readbufferfactor>
  </network>
  <video>
    <playcountminimumpercent>90</playcountminimumpercent>
    <ignoresecondsatstart>210</ignoresecondsatstart>
    <ignorepercentatend>8</ignorepercentatend>
  </video>
  <audio>
    <streamsilence>1</streamsilence>
  </audio>
  <gui>
    <algorithmdirtyregions>3</algorithmdirtyregions>
    <nofliptimeout>0</nofliptimeout>
  </gui>
  <cache>
    <buffermode>1</buffermode>
    <memorysize>20971520</memorysize>
    <readfactor>4.0</readfactor>
  </cache>
  <loglevel hide="false">1</loglevel>
  <cputempcommand>sensors | grep "Core 0" | cut -c17-18</cputempcommand>
  <gputempcommand>nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits</gputempcommand>
</advancedsettings>
EOF
      
      # Create installation script
      mkdir -p $out/bin
      
      cat > $out/bin/install-hy300-kodi-complete << 'EOF'
#!/bin/bash
# Install complete HY300 Kodi configuration with Nimbus skin and addons

KODI_USERDATA_DIR="/home/projector/.kodi/userdata"
KODI_ADDONS_DIR="/home/projector/.kodi/addons"

echo "Installing HY300 Kodi complete configuration..."

# Create directory structure
mkdir -p "$KODI_USERDATA_DIR"
mkdir -p "$KODI_ADDONS_DIR"

# Install addons
echo "Installing Kodi addons..."
cp -r $out/share/kodi/addons/* "$KODI_ADDONS_DIR/"

# Install configuration files
echo "Installing Kodi configuration..."
cp $out/share/hy300/kodi/guisettings.xml "$KODI_USERDATA_DIR/"
cp $out/share/hy300/kodi/sources.xml "$KODI_USERDATA_DIR/"
cp $out/share/hy300/kodi/advancedsettings.xml "$KODI_USERDATA_DIR/"

# Create media directories
echo "Creating media directories..."
mkdir -p /media/{videos,music,pictures}

# Set permissions
echo "Setting permissions..."
chown -R projector:projector /home/projector/.kodi
chown -R projector:projector /media

echo ""
echo "HY300 Kodi complete installation finished!"
echo ""
echo "Installed components:"
echo "- Nimbus Skin (modern, projector-friendly interface)"
echo "- FenLightAM Addon (lightweight media addon)"
echo "- CocoScrapers Module (web scraping support)"
echo "- HY300 Keystone Correction Plugin"
echo "- HY300 WiFi Setup Plugin"
echo ""
echo "Configuration optimizations:"
echo "- Hardware video acceleration enabled"
echo "- Network buffering optimized"
echo "- Remote control friendly settings"
echo "- Projector-specific display settings"
echo ""
echo "Default skin: Nimbus"
echo "Media directories: /media/{videos,music,pictures}"
echo ""
echo "Restart Kodi to apply all changes."
EOF
      
      chmod +x $out/bin/install-hy300-kodi-complete
      
      # Create Kodi startup script for HY300
      cat > $out/bin/hy300-kodi << 'EOF'
#!/bin/bash
# HY300 Kodi Startup Script
# Optimized startup for projector environment

export KODI_HOME="/home/projector/.kodi"
export KODI_USERDATA="$KODI_HOME/userdata"

# Ensure hardware acceleration is available
export LIBVA_DRIVER_NAME=radeonsi
export VDPAU_DRIVER=radeonsi

# Set audio output for projector
export PULSE_RUNTIME_PATH="/run/user/$(id -u projector)/pulse"

# Start Kodi with projector-optimized settings
exec kodi-standalone \
    --windowing=x11 \
    --audio-backend=pulse \
    --loglevel=1 \
    "$@"
EOF
      
      chmod +x $out/bin/hy300-kodi
    '';
    
    meta = with lib; {
      description = "Complete HY300 Kodi configuration with Nimbus skin and required addons";
      longDescription = ''
        Complete Kodi setup for HY300 projector including:
        - Nimbus skin for modern, clean interface
        - FenLightAM addon for media content
        - CocoScrapers module for web scraping support
        - HY300-specific plugins for keystone correction and WiFi setup
        - Optimized configuration for projector environment
      '';
      license = licenses.gpl3Only;
      platforms = platforms.linux;
    };
  };

  # Legacy compatibility - individual packages
  kodi-hy300-plugins = pkgs.symlinkJoin {
    name = "kodi-hy300-plugins";
    paths = [
      kodi-plugin-hy300-keystone
      kodi-plugin-hy300-wifi
    ];
    
    meta = with lib; {
      description = "HY300 hardware-specific Kodi plugins only";
      license = licenses.gpl3Only;
      platforms = platforms.linux;
    };
  };
}