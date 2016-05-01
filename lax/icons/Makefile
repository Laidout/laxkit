# To regenerate the icons, remove icons.stamp from this 
# directory and type 'make'.

all:
	@if [ ! -e icons.stamp ] ; then \
			echo "Generating icons..." && \
			./makeimages.py && \
			touch icons.stamp; \
		else echo "Icons already made! Remove icons.stamp and run make to redo.\n Or do ./makeimages.py IconName to just make one."; fi

