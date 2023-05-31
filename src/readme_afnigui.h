static char * readme_afnigui[] = {
   "================================================================================\n" ,
   "                ----===|  Usage Tips for the AFNI GUI  |===----\n" ,
   "================================================================================\n" ,
   "Tip #1:\n" ,
   "  When the AFNI sunrise logo is displayed in the square to right of the 'done'\n" ,
   "  button, this means that something is happening that might take a long time\n" ,
   "  (e.g., reading a big file).  The AFNI GUI will not respond to clicks or\n" ,
   "  keystrokes during this time.\n" ,
   "--------------------------------------------------------------------------------\n" ,
   "                    ***** AFNI For Absolute Beginners *****\n" ,
   "\n" ,
   "(1) To use AFNI, you must have some data stored in a format that the program\n" ,
   "    understands.\n" ,
   "   (a) The major formats for which AFNI is programmed are:\n" ,
   "       ++ AFNI formatted datasets, in .HEAD and .BRIK pairs of files;\n" ,
   "       ++ NIfTI-1 formatted datasets, in .nii or .nii.gz files.\n" ,
   "   (b) When you start AFNI, any datasets in the current directory will be ready\n" ,
   "       for viewing.  If there are no datasets to read, AFNI will pop up a\n" ,
   "       message.  At this point, you can do one of 2 things:\n" ,
   "       ++ Quit AFNI, change directory ('cd') to a directory with some datasets,\n" ,
   "          and restart the program (this is what I always do); or,\n" ,
   "       ++ Use the 'DataDir Read' button in the second column of the main AFNI\n" ,
   "          controller to navigate to a directory with some datasets.\n" ,
   "       When there is no data, AFNI creates a 'dummy' dataset for you to look at,\n" ,
   "       if you want to play with the image viewers.\n" ,
   "   (c) To open an image viewer, click on one of the 'Image' buttons in the left\n" ,
   "       column of the main AFNI controller.\n" ,
   "       ++ Left-click in the image to move the crosshairs around.\n" ,
   "       ++ Click or click-drag in the slider below the image to change slice.\n" ,
   "       ++ The controls to the right of the image let you adjust the image\n" ,
   "          contrast, brightness, zoom factor, etc.\n" ,
   "       ++ The controls to the bottom of the image let you carry out various\n" ,
   "          operations on the image, such as saving to JPEG format, and making\n" ,
   "          a montage (array) of slice images.\n" ,
   "   (d) If a dataset has more than one 3D volume in it (usually that means it\n" ,
   "       has a time axis, like an FMRI EPI dataset), then you can use the 'Graph'\n" ,
   "       buttons to view an array of graphs of the voxel data time series.\n" ,
   "       ++ The crosshairs in the image viewer that corresponds to the graph\n" ,
   "          viewer change to a small box to indicate which voxels are the source\n" ,
   "          of the data in the graph array.\n" ,
   "   (e) The viewpoint of multiple image and graph viewers (the focus voxel at the\n" ,
   "       crosshairs) is always locked together -- Left-click in one image to move\n" ,
   "       the crosshairs, and all other open image and graph viewers will jump, too.\n" ,
   "       ++ Moving the time index in a graph viewer (by clicking in the central\n" ,
   "          sub-graph, or by using the 'Index' control in the main AFNI window)\n" ,
   "          will cause all the image and graph viewers to display at the new\n" ,
   "          time index.\n" ,
   "   (f) At any moment, you can have one underlay dataset (grayscale) and one\n" ,
   "       overlay dataset (color).  To choose which datasets are visible, use\n" ,
   "       the 'UnderLay' and 'OverLay' button in the second column of the AFNI\n" ,
   "       controller.\n" ,
   "       ++ To control whether the color overlay is visible, click on the \n" ,
   "          'See OverLay' toggle button.\n" ,
   "       ++ To manage how the numbers in the overlay dataset are converted into\n" ,
   "          colors, click on 'Define OverLay' to open up a new control panel.\n" ,
   "       ++ The active underlay and overlay dataset names are shown in the title\n" ,
   "          bar of the AFNI controller window.\n" ,
   "   (g) Don't be afraid of the software!  The popup hints will help.  The button\n" ,
   "       'BHelp' will give you longer help about individual buttons -- click on\n" ,
   "       the BHelp button, then click on any AFNI button to get a small text box\n" ,
   "       with more help than the simple popup hints.\n" ,
   "   (h) Sample datasets and all the AFNI course materials are in the big file\n" ,
   "       CD.tgz (more than 1 Gbyte) at\n" ,
   "          https://afni.nimh.nih.gov/pub/dist/edu/data\n" ,
   "       And don't forget the AFNI message board at\n" ,
   "          https://discuss.afni.nimh.nih.gov\n" ,
   "--------------------------------------------------------------------------------\n" ,
   "             ***** Cursor Shapes and Right-Click Popup Menus *****\n" ,
   "\n" ,
   "(2) On most systems, the X11 cursor used in AFNI is an orange arrow pointing\n" ,
   "    at about '10:30' on the clock face.  However, the arrow becomes yellow\n" ,
   "    and points more steeply, to about '11:00', when it is over a location that\n" ,
   "    has a hidden Right-Click popup menu.  Some of these are:\n" ,
   "   (a) Over the logo square just to the right of the 'done' button in an\n" ,
   "       AFNI controller.\n" ,
   "   (b) Over the labels at the top of the threshold slider and color bar in\n" ,
   "       the 'Define Overlay' control panel.\n" ,
   "   (c) On the label to the left of a drop-down 'option menu'; e.g., 'Xhairs'\n" ,
   "       or 'ULay' (for these menus, the cursor will not change shape).\n" ,
   "   (d) Over the color bar itself, when it is in continuous 'colorscale' mode\n" ,
   "       ('**') -- but not when it is in discrete color panel mode.\n" ,
   "   (e) Over the image viewer main sub-window, which holds the image itself.\n" ,
   "   (f) Over the image viewer intensity bar, just to the right of the image.\n" ,
   "   (g) Over the 'Save' button in the image viewer.\n" ,
   "   (h) Over the 'crop' button in the image viewer.\n" ,
   "   (i) Over the sub-graphs in the time series dataset graph viewer\n" ,
   "       (pops up some statistics about the data in the sub-graph).\n" ,
   "   (j) Over the coordinate display in the upper left corner of an AFNI\n" ,
   "       controller.\n" ,
   "   (k) In the 'Render Dataset' plugin, over the 'See Xhairs' and 'Accumulate'\n" ,
   "       toggle buttons.  The 'Accumulate' popup lets you enter a text label\n" ,
   "       to be drawn in the rendered image viewer -- the actual display of labels\n" ,
   "       (size and location) is controlled from the image viewer intensity bar\n" ,
   "       popup menu -- item (e) above.  Right-Click lets you change the label\n" ,
   "       for newly rendered images, whereas Shift+Right-Click lets you change\n" ,
   "       the label for all previously rendered images as well as new images.\n" ,
   "   (l) In the 'Clusters' report panel, right-clicking in the upper part of\n" ,
   "       the window (where the textual summary is), will let you choose how\n" ,
   "       the 'Histog' option will plot the histogram from the 'Aux.Dset'.\n" ,
   "       ('Histog' is selected from the menu whose initial choice is 'Mean'.)\n" ,
   "--------------------------------------------------------------------------------\n" ,
   "              ***** Hidden Clicks to Activate AFNI Features *****\n" ,
   "\n" ,
   "(3) Other 'special' clicks that aren't obvious:\n" ,
   "   (a) Left-clicking on the 'Image' or 'Graph' button for an already open\n" ,
   "       AFNI viewer will bring that viewer's window back up to the top of\n" ,
   "       the X11 window stacking order.\n" ,
   "   (b) Right-clicking on the 'Image' or 'Graph' button for an already open\n" ,
   "       AFNI viewer will bring that viewer's window over to the place where\n" ,
   "       you just clicked -- this is useful for finding lost viewers.\n" ,
   "   (c) Right-clicking on the 'DataDir' button in the AFNI controller will\n" ,
   "       un-hide (or re-hide) the obsolescent 'Define Markers' button.\n" ,
   "   (d) Left-clicking on the logo at the lower left of a graph viewer will\n" ,
   "       make the logo and menu buttons disappear -- this is intended to help\n" ,
   "       you make a clean-looking snapshot of the window.  Left-click again\n" ,
   "       in the same region at the lower left to un-hide the logo and buttons.\n" ,
   "   (e) Right-clicking on the graph viewer logo or on the image viewer 'Disp'\n" ,
   "       button (i.e., the things at the lower left of each viewer window) will\n" ,
   "       bring the parent AFNI controller back to the top of the X11 window\n" ,
   "       stacking order.\n" ,
   "   (f) In an image viewer window, if you press-and-hold-down the Left mouse\n" ,
   "       button, then drag the cursor vertically and horizontally (while still\n" ,
   "       holding the Left button down), the contrast and brightness of the\n" ,
   "       grayscale underlay changes as you move the mouse.\n" ,
   "   (g) To exit AFNI quickly when multiple controllers are open, hold down the\n" ,
   "       Shift key when you press the 'done' button (in any AFNI controller).\n" ,
   "       The usual requirement of pressing 'done' twice within 5 second is\n" ,
   "       skipped, and all AFNI controllers exit immediately.\n" ,
   "   (h) Left-clicking in the logo square to the right of the AFNI controller\n" ,
   "       'done' button will cause the AFNI splash screen to popup.  You can\n" ,
   "       close the splash screen window again via the usual X11 controls on\n" ,
   "       the window border, or by left-clicking again in the logo square.\n" ,
   "--------------------------------------------------------------------------------\n" ,
   "               ***** Keyboard Shortcuts: Image Viewer Window *****\n" ,
   "\n" ,
   "(4) The AFNI image viewer has many keyboard shortcuts.  Help for these can be\n" ,
   "    found by using 'BHelp' and then clicking on the image sub-window itself.\n" ,
   "    The shortcuts are used by putting the mouse cursor over the image sub-window\n" ,
   "    and pressing the keyboard key.\n" ,
   "      a = fix window aspect ratio        (= Left-click in intensity bar)\n" ,
   "      c = enter cropping mode            (= clicking the 'crop' button)\n" ,
   "      D = open Disp control panel        (= clicking the 'Disp' button)\n" ,
   "      i = make the image sub-window smaller inside the overall viewer\n" ,
   "      I = make the image sub-window larger\n" ,
   "      l = left-right mirror image        (= '+LR Mirror' on Disp panel)\n" ,
   "      m = toggle 'Min-to-Max' on/off     (= 'Min-to-Max' or '2%-to-98%' on Disp)\n" ,
   "      M = open Montage control panel     (= clicking the 'Mont' button)\n" ,
   "      o = color Overlay on/off           (= 'See Overlay' in AFNI controller)\n" ,
   "      p = toggle panning mode            (= clicking the 'pan' button)\n" ,
   "      q = close window                   (= clicking the 'done' button)\n" ,
   "      r = like 'v', but image 'rebounds' when it reaches end of count\n" ,
   "      R = like 'r', but backwards order\n" ,
   "      s = sharpen underlay image         (= 'Sharpen' on the Disp control panel)\n" ,
   "      S = save image                     (= clicking the 'Save' button)\n" ,
   "      u = toggle background to be from Underlay or Overlay dataset\n" ,
   "      v = automatically change images to make a 'video' (forwards order)\n" ,
   "      V = same as 'v' but backwards order\n" ,
   "      z = zoom out                       (= zoom down-arrow button)\n" ,
   "      Z = zoom in                        (= zoom up-arrow button)\n" ,
   "      [ = time index down by 1\n" ,
   "      ] = time index up by 1\n" ,
   "      { = lower AFNI statistical threshold slider\n" ,
   "      } = raise AFNI statistical threshold slider\n" ,
   "      > = PageUp = move forward one slice in image viewer\n" ,
   "      < = PageDown = move backward one slice in image viewer\n" ,
   "      # = toggle checkerboard pattern from underlay and overlay datasets\n" ,
   "      3 = turn checkerboard pattern off\n" ,
   "    Keyboard arrow keys = move crosshairs (normal operation)\n" ,
   "                        = pan zoomed window (when 'pan' is on)\n" ,
   "    Shift+arrow keys    = pan cropped region around\n" ,
   "    Ctrl+arrow keys     = expand/shrink cropped region\n" ,
   "    Home key            = center zoomed window on current crosshair location\n" ,
   "    Shift+Home          = center cropped region on current crosshair location\n" ,
   "    Del = undo in the Drawing plugin\n" ,
   "     F2 = turn the Drawing pencil on\n" ,
   "     F3 = subtract 1 from the Drawing plugin value\n" ,
   "     F4 = add 1 to the Drawing plugin value\n" ,
   "     F5 = Meltdown!\n" ,
   "     F6 and higher are not (yet) defined.\n" ,
   "--------------------------------------------------------------------------------\n" ,
   "               ***** Keyboard Shortcuts: Graph Viewer Window *****\n" ,
   "\n" ,
   "(5) The AFNI graph viewer also has many keyboard shortcuts.\n" ,
   "      a = autoscale the graphs (this one time)\n" ,
   "      A = autoscale the graphs (every time they are redrawn)\n" ,
   "      b = switch graph baseline mode (between Separate, Common, and Global)\n" ,
   "      B = switch on/off Boxes graphs instead of line graphs\n" ,
   "      g = decrease vertical grid spacing in graphs\n" ,
   "      G = increase vertical grid spacing in graphs\n" ,
   "      h = draw dotted horizontal line at zero level in graphs\n" ,
   "      i = decrease graph 'ignore' level by 1\n" ,
   "      I = increase graph 'ignore' level by 1\n" ,
   "      l = move to last time point\n" ,
   "      L = turn AFNI logo on or off (but why would you want it OFF?!)\n" ,
   "      m = reduce matrix size of sub-graphs by 1\n" ,
   "      M = increase matrix size of sub-graphs by 1\n" ,
   "      q = quit = Opt->Done\n" ,
   "    r/R = 'rebound' up or down in time\n" ,
   "      S = save an image of the graph window to a file\n" ,
   "      t = show text (numbers) instead of graphs\n" ,
   "    v/V = 'video' up or down in time\n" ,
   "      w = write data from central sub-graph to a 1D file\n" ,
   "      z = change slice number by 1 downwards (= keyboard Page Down key)\n" ,
   "      Z = change slice number by 1 upwards   (= keyboard Page Up key)\n" ,
   "      - = scale the graphs down (vertically)\n" ,
   "      + = scale the graphs up\n" ,
   "      1 = move to first time point\n" ,
   " < or [ = move back in time by 1 point  (= keyboard Left arrow)\n" ,
   " > or ] = move up in time by 1 point    (= keyboard Right arrow)\n" ,
   "      N = after typing the 'N', type a number, then press Enter;\n" ,
   "          the matrix of sub-graphs will change to the number given immediately\n" ,
   "     F5 = Meltdown!\n" ,
   "--------------------------------------------------------------------------------\n" ,
   "            ***** Keyboard Shortcuts in the Threshold Slider *****\n" ,
   "\n" ,
   "(6) Fine control over the threshold slider is hard with the mouse.  You can\n" ,
   "    move the slider with the following keystrokes, after clicking in the\n" ,
   "    slider's 'thumb' to move the keyboard input focus to this control.\n" ,
   "      Down arrow = reduce slider by 1 in the 4th significant place\n" ,
   "      Up arrow   = increase slider by 1 in the 4th significant place\n" ,
   "      Page Down  = reduce slider by 1 in the 3rd significant place\n" ,
   "      Page Up    = increase slider by 1 in the 3rd significant place\n" ,
   "      Home       = drop slider to 0\n" ,
   "      End        = move slider to top\n" ,
   "    The Left and Right arrow keys can similarly be used in the image viewer\n" ,
   "    slice index slider to move 1 slice backward or forward at a time.\n" ,
   "--------------------------------------------------------------------------------\n" ,
   NULL } ;
#define NUM_readme_afnigui 227
