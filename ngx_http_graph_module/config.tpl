#
MY_PREFIX="%%prefix%%"
PATH_LOC1="$MY_PREFIX/libs"
PATH_LOC2="/usr/local/atlas/lib"

RPATH="-Wl,-rpath=${PATH_LOC1}:${PATH_LOC2}:/usr/local/lib:/usr/lib:"

MY_CFLAGS=" -O2 -fopenmp -std=gnu++0x -fPIC $RPATH "
#MY_CFLAGS=" -O2 -fopenmp -DCPU_ONLY -std=gnu++0x -fPIC $RPATH "

LIBS="-L${PATH_LOC1} -L${PATH_LOC2} -lstcv_cicf_detect -lstcv_dataio_lib -lstcv_pieta_lib -lcaffe -lold_recwrapper_lib -lopencv_imgproc -lopencv_calib3d -lopencv_contrib -lopencv_core -lopencv_features2d -lopencv_flann -lopencv_highgui -lopencv_legacy -lopencv_ml -lopencv_nonfree -lopencv_objdetect -lopencv_photo -lopencv_stitching -lopencv_superres -lopencv_ts -lopencv_video -lopencv_videostab -lopencv_gpu -llibjasper -llibjpeg -llibpng -llibtiff -lIlmImf -lcblas -latlas -lbsl -lconfig -lullib -lspreg -ljson -lm -lcurl -ldl -lglog -lgflags -lboost_system "
#-----------------------------------------------
#
CORE_LIBS="$CORE_LIBS $LIBS"
LINK="gcc -std=c++11 ${MY_CFLAGS}"
ngx_addon_name=ngx_http_graph_module
HTTP_MODULES="$HTTP_MODULES ngx_http_graph_module"
NGX_ADDON_SRCS="$NGX_ADDON_SRCS ../ngx_http_graph_module/ngx_http_graph_module.cpp ../ngx_http_graph_module/PluginSlot.cpp"

