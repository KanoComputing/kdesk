//
//  kbfsaver.cpp - Implement a simple screen saver drawing directly to the framebuffer
//                 With a bit more time investing on this it could also use OpenGL using a top z-order layer
//
//  This code is highly based on this great tutorial:
//
//   * http://raspberrycompote.blogspot.com.es/2013/01/low-level-graphics-on-raspberry-pi-part.html
//

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

int draw_something (char *screen_surface, long int screen_size)
{
  // just fill upper half of the screen with something
  memset (screen_surface, 0x12, screen_size/2);
  
  // and lower half with something else
  memset (screen_surface + screen_size/2, 0x10, screen_size/2);
}

int main(int argc, char* argv[])
{
  unsigned long refresh_rate = 1;
  struct fb_var_screeninfo vinfo;
  struct fb_fix_screeninfo finfo;
  long int screen_size = 0;
  char *fbp = 0;
  int idling = 1;
  int fbfd = 0;
  int n;

  // Open the file for reading and writing
  fbfd = open("/dev/fb0", O_RDWR);
  if (!fbfd) {
    printf("Error: cannot open framebuffer device.\n");
    return(1);
  }
  printf("The framebuffer device was opened successfully.\n");

  // Get fixed screen information
  if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
    printf("Error reading fixed information.\n");
  }

  // Get variable screen information
  if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
    printf("Error reading variable information.\n");
  }
  printf("%dx%d, %d bpp\n", vinfo.xres, vinfo.yres, 
         vinfo.bits_per_pixel );

  // map framebuffer to user memory 
  screen_size = finfo.smem_len;
  fbp = (char*)mmap(0, 
                    screen_size, 
                    PROT_READ | PROT_WRITE, 
                    MAP_SHARED, 
                    fbfd, 0);

  if ((int)fbp == -1) {
    printf("Failed to mmap.\n");
  }
  else {

    // Open access to input devices (keyboard / mouse)
    int fdkbd, fdmouse;
    char buf[128];

    // Initial startup delay to settle relax XServer events
    usleep (1000 * 1000);

    fdkbd = open("/dev/input/event1", O_RDWR | O_NOCTTY | O_NDELAY);
    if (fdkbd == -1) {
      printf("open_port: Unable to open /dev/input/event1");
      idling = 0;
    }
    else {
      // Turn off blocking for reads, use (fd, F_SETFL, FNDELAY) if you want that
      fcntl(fdkbd, F_SETFL, O_NONBLOCK);
    }

    fdmouse = open("/dev/input/event0", O_RDWR | O_NOCTTY | O_NDELAY);
    if (fdmouse == -1) {
      printf("open_port: Unable to open /dev/input/event2");
      idling = 0;
    }
    else {
      // Turn off blocking for reads, use (fd, F_SETFL, FNDELAY) if you want that
      fcntl(fdmouse, F_SETFL, O_NONBLOCK);
    }

    // Let's draw something on the screen, repeatedly,
    // until we receive input from either the keyboard or mouse
    while (idling)
      {
	draw_something (fbp, screen_size);

	// If there is an input event from keyboard or mouse, stop now
	n = read(fdkbd, (void*)buf, 255);
	if (n > 0) {
	  idling = 0;
	}

	n = read(fdmouse, (void*)buf, 255);
	if (n > 0) {
	  idling = 0;
	}

	// wait half a second
	usleep (1000 * refresh_rate);
      }
  }

  printf ("cleanup and exit");
  munmap(fbp, screen_size);
  close(fbfd);

  // Refresh the - most possibly - running XServer desktop
  system ("xrefresh");
  return 0;
}
