

Introduction
-------------------

CodeMagic is an advanced source code editor plugin for TinyMCE.

It integrates the CodeMirror library for syntax coloring, and the JSBeautifier library for code formating and indentation.



Dependencies
-------------------

CodeMagic: http://codemagic.sutulustus.com
WYSIWYG: http://drupal.org/project/wysiwyg
TinyMCE: http://tinymce.moxiecode.com



Installation
-------------------
1. Install the WYSIWYG module as normal
 - http://drupal.org/documentation/install/modules-themes/modules-7



2. Download the TinyMCE editor and extract the archive into a new folder in the following location:

sites/all/libraries/tinymce

So the actual library can be found at:
sites/all/libraries/tinymce/jscripts/tiny_mce/tiny_mce.js



3. Download the CodeMagic plugin into:

sites/all/libraries/tinymce/jscripts/tiny_mce/plugins/codemagic

So the actual plugin can be found at:
sites/all/libraries/tinymce/jscripts/tiny_mce/plugins/codemagic/editor_plugin.js



4. Go to your WYSIWYG profiles under /admin/config/content/wysiwyg, and enable "CodeMagic" under "Buttons and Plugins". This will give you a new button in your TinyMCE toolbar.




Development is sponsored by TinymceSupport.com



