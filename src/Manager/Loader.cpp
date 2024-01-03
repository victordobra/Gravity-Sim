#include "Loader.hpp"
#include <stdio.h>

namespace gsim {
	size_t LoadPoints(const char* fileName, Point* points) {
		// Open the file input stream
		FILE* fileInput = fopen(fileName, "r");
		if(!fileInput)
			return 0;
		
		// Keep reading points until the file is over
		Point point;
		size_t pointCount = 0;
		while(fscanf(fileInput, "%f%f%f%f%f", &point.pos.x, &point.pos.y, &point.vel.x, &point.vel.y, &point.mass) == 5) {
			// Write the point to the point array, if one was given
			if(points)
				*points++ = point;
			
			// Increment the point count
			++pointCount;
		}

		// Close the file stream
		fclose(fileInput);

		// Return the point count
		return pointCount;
	}
}