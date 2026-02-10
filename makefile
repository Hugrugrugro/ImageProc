all: image signal FFT main
	g++ lib/ImageProcessing.o lib/SignalProcessing.o lib/FFT.o lib/main.o -o bin/PPG.out -L/usr/lib/x86_64-linux-gnu -lopencv_stitching -lopencv_alphamat -lopencv_aruco -lopencv_barcode -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_dnn_objdetect -lopencv_dnn_superres -lopencv_dpm -lopencv_face -lopencv_freetype -lopencv_fuzzy -lopencv_hdf -lopencv_hfs -lopencv_img_hash -lopencv_intensity_transform -lopencv_line_descriptor -lopencv_mcc -lopencv_quality -lopencv_rapid -lopencv_reg -lopencv_rgbd -lopencv_saliency -lopencv_shape -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_superres -lopencv_optflow -lopencv_surface_matching -lopencv_tracking -lopencv_highgui -lopencv_datasets -lopencv_text -lopencv_plot -lopencv_ml -lopencv_videostab -lopencv_videoio -lopencv_viz -lopencv_wechat_qrcode -lopencv_ximgproc -lopencv_video -lopencv_xobjdetect -lopencv_objdetect -lopencv_calib3d -lopencv_imgcodecs -lopencv_features2d -lopencv_dnn -lopencv_flann -lopencv_xphoto -lopencv_photo -lopencv_imgproc -lopencv_core


image: src/ImageProcessing.cpp
	g++ -c src/ImageProcessing.cpp -o lib/ImageProcessing.o -I./include -I/usr/include/opencv4
	
signal: src/SignalProcessing.cpp
	g++ -c src/SignalProcessing.cpp -o lib/SignalProcessing.o -I./include -I/usr/include/opencv4

FFT: src/FFT.cpp
	g++ -c src/FFT.cpp -o lib/FFT.o -I./include

main: src/main.cpp
	g++ -c src/main.cpp -o lib/main.o -I/usr/include/opencv4 -I./include/

clean:
	rm lib/*.o
	rm bin/*.exe