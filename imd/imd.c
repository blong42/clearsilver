/*
 * Copyright 2001-2004 Brandon Long
 * All Rights Reserved.
 *
 * ClearSilver Templating System
 *
 * This code is made available under the terms of the ClearSilver License.
 * http://www.clearsilver.net/license.hdf
 *
 */

/* Bring in gd library functions */
#include "gd.h"

/* Bring in standard I/O so we can output the PNG to a file */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <time.h>
#include <ctype.h>

#include "ClearSilver.h"

/* from httpd util.c : made infamous with Roy owes Rob beer. */
static char *months[] = {
  "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};

int find_month(char *mon) {
  register int x;

  for(x=0;x<12;x++)
    if(!strcmp(months[x],mon))
      return x;
  return -1;
}

int later_than(struct tm *lms, char *ims) {
  char *ip;
  char mname[256];
  int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0, x;

  /* Whatever format we're looking at, it will start
   * with weekday. */
  /* Skip to first space. */
  if(!(ip = strchr(ims,' ')))
    return 0;
  else
    while(isspace(*ip))
      ++ip;

  if(isalpha(*ip)) {
    /* ctime */
    sscanf(ip,"%25s %d %d:%d:%d %d",mname,&day,&hour,&min,&sec,&year);
  }
  else if(ip[2] == '-') {
    /* RFC 850 (normal HTTP) */
    char t[256];
    sscanf(ip,"%s %d:%d:%d",t,&hour,&min,&sec);
    t[2] = '\0';
    day = atoi(t);
    t[6] = '\0';
    strcpy(mname,&t[3]);
    x = atoi(&t[7]);
    /* Prevent wraparound from ambiguity */
    if(x < 70)
      x += 100;
    year = 1900 + x;
  }
  else {
    /* RFC 822 */
    sscanf(ip,"%d %s %d %d:%d:%d",&day,mname,&year,&hour,&min,&sec);
  }
  month = find_month(mname);

  if((x = (1900+lms->tm_year) - year))
    return x < 0;
  if((x = lms->tm_mon - month))
    return x < 0;
  if((x = lms->tm_mday - day))
    return x < 0;
  if((x = lms->tm_hour - hour))
    return x < 0;
  if((x = lms->tm_min - min))
    return x < 0;
  if((x = lms->tm_sec - sec))
    return x < 0;

  return 1;
}



int gif_size (char *file, int *width, int *height)
{
  UINT8 data[256];
  int fd;
  int blen;

  *width = 0; *height = 0;
  fd = open (file, O_RDONLY);
  if (fd == -1)
    return -1;

  blen = read(fd, data, sizeof(data));
  close(fd);

  if (blen < 10) return -1;
  if (strncmp(data, "GIF87a", 6) && strncmp(data, "GIF89a", 6))
    return -1;

  *width = data[6] + data[7]*256;
  *height = data[8] + data[9]*256;

  return 0;
}

int jpeg_size (char *file, int *width, int *height)
{
  UINT8 data[64*1024];
  int blen;
  int fd;
  int pos;
  int length;
  UINT8 tag, marker;


  *width = 0; *height = 0;
  fd = open (file, O_RDONLY);
  if (fd == -1)
    return -1;

  blen = read(fd, data, sizeof(data));
  close(fd);
  pos = 2;
  while (pos+8 < blen)
  {
    tag = data[pos+0];
    if (tag != 0xff) return -1;
    marker = data[pos+1];
    length = data[pos+2] * 256 + data[pos+3] + 2;
    if (marker >= 0xc0 && marker <= 0xcf && marker != 0xc4 && 
	marker != 0xc8 && marker != 0xcc)
    {
      *height = data[pos+5] * 256 + data[pos+6];
      *width = data[pos+7] * 256 + data[pos+8];
      ne_warn("%s: %dx%d", file, *width, *height);
      return 0;
    }
    pos += length;
  }
  return -1;
}

int isdir(char *dir) {
  struct stat statinfo;
  if ( stat(dir, &statinfo) != 0) {
    return 0;
  }

  return S_ISDIR(statinfo.st_mode);
}

int create_directories(char *fullpath) {
  char s[4000];
  char *last_slash;
  int last_slash_pos;

  if ((fullpath == NULL) || (strlen(fullpath) > 4000)) {
    return 1;
  }

  last_slash = strrchr(fullpath,'/');
  last_slash_pos = (last_slash - fullpath);
  /* fprintf(stderr,"dira(%d): %s\n", last_slash_pos,fullpath); */

  if (last_slash_pos > 2) {
    strncpy(s,fullpath,last_slash_pos);
    s[last_slash_pos] = 0;
    /* fprintf(stderr,"dir: %s\n", s); */

    if (!isdir(s)) {
      char s2[4000];
      sprintf(s2,"mkdir -p %s", s);
      return system(s2);
    }

  } else {
    return 1;
  }

  return 0;
}

NEOERR *rotate_image(char *path, char *file, int degree, char *rpath)
{
  char cmd[256];
  char nfile[_POSIX_PATH_MAX];
  char ofile[_POSIX_PATH_MAX];
  char *ch, *opt;
  int is_jpeg = 0;
  struct stat s;
  int r;

  snprintf (ofile, sizeof(ofile), "%s/%s", path, file);
  snprintf (rpath, _POSIX_PATH_MAX, "%s/%s", path, file);
  ch = strrchr(rpath, '.');
  if ((!strcasecmp(ch, ".jpg")) ||
      (!strcasecmp(ch, ".jpeg")) ||
      (!strcasecmp(ch, ".thm")))
  {
    is_jpeg = 1;
  } 
  else if (strcasecmp(ch, ".gif"))
  {
    return nerr_raise(NERR_ASSERT, "Only support gif/jpeg for rotation, ext %s", 
	ch);
  }
  *ch = '\0';
  if (degree == 90)
  {
    strcat(rpath, "_r");
    opt = "-cw";
  }
  else if (degree == -90)
  {
    strcat(rpath, "_l");
    opt = "-ccw";
  }
  else if (degree == 180)
  {
    strcat(rpath, "_u");
    opt = "-rotate180";
  }
  else 
  {
    return nerr_raise(NERR_ASSERT, "currently only support 90/-90/180 rotations");
  }
  if (is_jpeg)
  {
    strcat(rpath, ".jpg");
    snprintf(cmd, sizeof(cmd), "djpeg -pnm %s | pnmflip %s | cjpeg -quality 85 > %s", ofile, opt, rpath);
  }
  else
  {
    strcat(rpath, ".gif");
    snprintf(cmd, sizeof(cmd), "giftopnm %s | pnmflip %s | ppmtogif > %s", ofile, opt, rpath);
  }
  /* already exists? */
  if (!stat(rpath, &s))
  {
    return STATUS_OK;
  }
  r = system(cmd);
  if (r) return nerr_raise_errno (NERR_SYSTEM, "%s returned %d", cmd, r);
  /* always save off the old file */
  snprintf (nfile, sizeof(nfile), "%s/%s.orig", path, file);
  if (stat(nfile, &s))
  {
    if (link(ofile, nfile))
      return nerr_raise_errno (NERR_SYSTEM, "Unable to link %s -> %s", ofile, nfile);
    unlink(ofile);
  }
  return STATUS_OK;
}

NEOERR *scale_and_display_image(char *fname,int maxW,int maxH,char *cachepath,
    int quality) 
{
  NEOERR *err = STATUS_OK;
  /* Declare the image */
  gdImagePtr src_im = 0;
  /* Declare input file */
  FILE *infile=0, *cachefile=0;
  int srcX,srcY,srcW,srcH;
  FILE *dispfile=0;
  struct stat s;

  /* if we can open the cachepath, then just print it */
  if (!stat(cachepath, &s) && s.st_size)
    cachefile = fopen(cachepath,"rb");
  if (cachefile) {
    /* we should probably stat the files and make sure the thumbnail
       is current */
    /* fprintf(stderr,"using cachefile: %s\n",cachepath); */
    dispfile = cachefile;
  } else {
    char cmd[1024];
    int factor=1;
    int l;
    int is_jpeg = 0, is_gif = 0;

    l = strlen(fname);
    if ((l>4 && !strcasecmp(fname+l-4, ".jpg")) ||
	(l>4 && !strcasecmp(fname+l-4, ".thm")) ||
	(l>5 && !strcasecmp(fname+l-5, ".jpeg")))
      is_jpeg = 1;
    else if (l>4 && !strcasecmp(fname+l-4, ".gif"))
      is_gif = 1;


    if (is_jpeg)
    {
      if (!quality)
      {
	if (!jpeg_size (fname, &srcW, &srcH))
	{
	  if ((srcW > maxW) || (srcH > maxH)) 
	  {
	    factor = 2;
	    if (srcW / factor > maxW)
	    {
	      factor = 4;
	      if (srcW / factor > maxW)
		factor = 8;
	    }
	  }
	}

	/* ne_warn("factor %d\n", factor); */
	snprintf (cmd, sizeof(cmd), "/usr/bin/djpeg -fast -scale 1/%d '%s' | /usr/bin/cjpeg -quality 60 -progressive -dct fast -outfile '%s'", factor, fname, cachepath);

	create_directories(cachepath);
	system(cmd);
	if (!stat(cachepath, &s) && s.st_size)
	  cachefile = fopen(cachepath,"rb");
	else
	  ne_warn("external command failed to create file\n");
      }
      if (cachefile) {
	dispfile = cachefile;

      } else /* no cachefile */ { 


	/* fprintf(stderr,"reading image\n"); */
	/* Read the image in */
	infile = fopen(fname,"rb");
	src_im = gdImageCreateFromJpeg(infile);
	srcX=0; srcY=0; srcW=src_im->sx; srcH=src_im->sy;


	/* figure out if we need to scale it */

	if ((maxW && srcW > maxW) || (maxH && srcH > maxH)) {
	  /* scale paramaters */
	  int dstX,dstY,dstW,dstH;
	  /* Declare output file */
	  FILE *jpegout;
	  gdImagePtr dest_im;
	  float srcAspect,dstAspect;

	  /* create the destination image */

	  dstX=0; dstY=0; 


	  srcAspect = ((float)srcW/(float)srcH);
	  dstAspect = ((float)maxW/(float)maxH);

	  if (srcAspect == dstAspect) {
	    /* they are the same aspect ratio */
	    dstW = maxW;
	    dstH = maxH;
	  } else if ( srcAspect > dstAspect ) {
	    /* if the src image has wider aspect ratio than the max */
	    dstW = maxW;
	    dstH = (int) ( ((float)dstW/(float)srcW) * srcH );
	  } else {
	    /* if the src image has taller aspect ratio than the max */
	    dstH = maxW;
	    dstW = (int) ( ((float)dstH/(float)srcH) * srcW );
	  }

#ifdef GD2_VERS
	  dest_im = gdImageCreateTrueColor(dstW,dstH);
#else
	  dest_im = gdImageCreate(dstW,dstH);
#endif

	  /* fprintf(stderr,"scaling to (%d,%d)\n",dstW,dstH); */

	  /* Scale it to the destination image */

	  gdImageCopyResized(dest_im,src_im,dstX,dstY,srcX,srcY,dstW,dstH,srcW,srcH);

	  /* fprintf(stderr,"scaling finished\n"); */

	  /* write the output image */
	  create_directories(cachepath);
	  jpegout = fopen(cachepath,"wb+");
	  if (!jpegout) {
	    jpegout = fopen("/tmp/foobar.jpg","wb+");
	  }
	  if (jpegout) {
	    gdImageJpeg(dest_im,jpegout,-1);
	    fflush(jpegout);

	    /* now print that data out the stream */
	    dispfile = jpegout;
	  } else {
	    return nerr_raise_errno(NERR_IO, "Unable to create output file: %s", cachepath);
	  }


	  gdImageDestroy(dest_im);

	} else {
	  /* just print the input file because it's small enough */
	  dispfile = infile;
	}

      }
    }
    else if (is_gif)
    {
      float scale = 1.0;
      if (!gif_size (fname, &srcW, &srcH))
      {
	if ((srcW > maxW) || (srcH > maxH)) 
	{
	  scale = 0.5;
	  if (srcW * scale > maxW)
	  {
	    scale = 0.25;
	    if (srcW * scale > maxW)
	      factor = 0.125;
	  }
	}
      }

      if (scale < 1.0)
      {
	snprintf (cmd, sizeof(cmd), "/usr/bin/giftopnm '%s' | /usr/bin/pnmscale  %5.3f | ppmquant 256 | ppmtogif > '%s'", fname, scale, cachepath);

	create_directories(cachepath);
	system(cmd);
	dispfile = fopen(cachepath,"rb");
	if (dispfile == NULL)
	  return nerr_raise_errno(NERR_IO, "Unable to open file: %s", cachepath);

      }
      else
      {
	dispfile = fopen(fname, "rb");
	if (dispfile == NULL)
	  return nerr_raise_errno(NERR_IO, "Unable to open file: %s", fname);
      }
    }
    else {
      dispfile = fopen(fname,"rb");
    }
  }

  /* the data in "dispfile" is going to be printed now */
  {

    char buf[8192];
    int count;

    if (!fstat(fileno(dispfile), &s) && s.st_size)
    {
      cgiwrap_writef("Content-Length: %ld\n\n", s.st_size);
    }
    else
    {
      cgiwrap_writef("\n");
    }
    
    fseek(dispfile,0,SEEK_SET);

    do {
      count = fread(buf,1,sizeof(buf),dispfile);
      if (count > 0) {
	err = cgiwrap_write(buf,count); 
      }
    } while (count > 0);

  }

  if (dispfile) fclose(dispfile); 
  if (src_im) gdImageDestroy(src_im);

  return nerr_pass(err);
}

NEOERR *load_images (char *path, ULIST **rfiles, char *partial, int descend)
{
  NEOERR *err = STATUS_OK;
  DIR *dp;
  struct dirent *de;
  int is_jpeg, is_gif, l;
  char fpath[_POSIX_PATH_MAX];
  char ppath[_POSIX_PATH_MAX];
  ULIST *files = NULL;

  if ((dp = opendir (path)) == NULL)
  {
    return nerr_raise(NERR_IO, "Unable to opendir %s: [%d] %s", path, errno, 
	strerror(errno));
  }

  if (rfiles == NULL || *rfiles == NULL)
  {
    err = uListInit(&files, 50, 0);
    if (err) return nerr_pass(err);
    *rfiles = files;
  }
  else
  {
    files = *rfiles;
  }

  while ((de = readdir (dp)) != NULL)
  {
    if (de->d_name[0] != '.')
    {
      snprintf(fpath, sizeof(fpath), "%s/%s", path, de->d_name);
      if (partial)
      {
	snprintf(ppath, sizeof(ppath), "%s/%s", partial, de->d_name);
      }
      else
      {
	strncpy(ppath, de->d_name, sizeof(ppath));
      }
      if (descend && isdir(fpath))
      {
	err = load_images(fpath, rfiles, ppath, descend);
	if (err) break;
      }
      else
      {
	l = strlen(de->d_name);
	is_jpeg = 0; is_gif = 0;

	if ((l>4 && !strcasecmp(de->d_name+l-4, ".jpg")) ||
	    (l>4 && !strcasecmp(de->d_name+l-4, ".thm")) ||
	    (l>5 && !strcasecmp(de->d_name+l-5, ".jpeg")))
	  is_jpeg = 1;
	else if (l>4 && !strcasecmp(de->d_name+l-4, ".gif"))
	  is_gif = 1;

	if (is_gif || is_jpeg) 
	{
	  err = uListAppend(files, strdup(ppath));
	  if (err) break;
	}
      }
    }
  }
  closedir(dp);
  if (err)
  {
    uListDestroy(&files, ULIST_FREE);
  }
  else
  {
    *rfiles = files;
  }
  return nerr_pass(err);
}

NEOERR *export_image(CGI *cgi, char *prefix, char *path, char *file)
{
  NEOERR *err;
  char buf[256];
  char num[20];
  int i = 0;
  int r, l;
  int width, height;
  char ipath[_POSIX_PATH_MAX];
  int is_jpeg = 0, is_gif = 0, is_thm = 0;

  l = strlen(file);
  if ((l>4 && !strcasecmp(file+l-4, ".jpg")) ||
      (l>5 && !strcasecmp(file+l-5, ".jpeg")))
    is_jpeg = 1;
  else if (l>4 && !strcasecmp(file+l-4, ".gif"))
    is_gif = 1;
  else if (l>4 && !strcasecmp(file+l-4, ".thm"))
    is_thm = 1;

  snprintf (buf, sizeof(buf), "%s.%d", prefix, i);
  err = hdf_set_value (cgi->hdf, prefix, file);
  if (err != STATUS_OK) return nerr_pass(err);
  snprintf (ipath, sizeof(ipath), "%s/%s", path, file);
  if (is_jpeg || is_thm)
    r = jpeg_size(ipath, &width, &height);
  else
    r = gif_size(ipath, &width, &height);
  if (!r)
  {
    snprintf (buf, sizeof(buf), "%s.width", prefix);
    snprintf (num, sizeof(num), "%d", width);
    err = hdf_set_value (cgi->hdf, buf, num);
    if (err != STATUS_OK) return nerr_pass(err);
    snprintf (buf, sizeof(buf), "%s.height", prefix);
    snprintf (num, sizeof(num), "%d", height);
    err = hdf_set_value (cgi->hdf, buf, num);
    if (err != STATUS_OK) return nerr_pass(err);
  }
  if (is_thm)
  {
    strcpy(ipath, file);
    strcpy(ipath+l-4, ".avi");
    snprintf(buf, sizeof(buf), "%s.avi", prefix);
    err = hdf_set_value (cgi->hdf, buf, ipath);
    if (err != STATUS_OK) return nerr_pass(err);
  }
  return STATUS_OK;
}

NEOERR *scale_images (CGI *cgi, char *prefix, int width, int height, int force)
{
  NEOERR *err;
  char num[20];
  HDF *obj;
  int i, x;
  int factor;

  obj = hdf_get_obj (cgi->hdf, prefix);
  if (obj) obj = hdf_obj_child (obj);
  while (obj)
  {
    factor = 1;
    i = hdf_get_int_value(obj, "height", -1);
    if (i != -1)
    {
      x = i;
      while (x > height)
      {
	/* factor = factor * 2;*/
	factor++;
	x = i / factor;
      }
      snprintf (num, sizeof(num), "%d", x);
      err = hdf_set_value (obj, "height", num);
      if (err != STATUS_OK) return nerr_pass (err);

      i = hdf_get_int_value(obj, "width", -1);
      if (i != -1)
      {
	i = i / factor;
	snprintf (num, sizeof(num), "%d", i);
	err = hdf_set_value (obj, "width", num);
	if (err != STATUS_OK) return nerr_pass (err);
      }
    }
    else
    {
      snprintf (num, sizeof(num), "%d", height);
      err = hdf_set_value (obj, "height", num);
      if (err != STATUS_OK) return nerr_pass (err);
      snprintf (num, sizeof(num), "%d", width);
      err = hdf_set_value (obj, "width", num);
      if (err != STATUS_OK) return nerr_pass (err);
    }
    obj = hdf_obj_next(obj);
  }
  return STATUS_OK;
}

int alpha_sort(const void *a, const void *b)
{
  char **sa = (char **)a;
  char **sb = (char **)b;

  /* ne_warn("%s %s: %d", *sa, *sb, strcmp(*sa, *sb)); */

  return strcmp(*sa, *sb);
}

static NEOERR *export_album_path(CGI *cgi, char *album, char *prefix)
{
  NEOERR *err = STATUS_OK;
  char *p, *l;
  int n = 0;
  char buf[256];

  l = album;
  p = strchr(album, '/');

  while (p != NULL)
  {
    *p = '\0';
    snprintf(buf, sizeof(buf), "%s.%d", prefix, n);
    err = hdf_set_value(cgi->hdf, buf, l);
    if (err) break;
    snprintf(buf, sizeof(buf), "%s.%d.path", prefix, n++);
    err = hdf_set_value(cgi->hdf, buf, album);
    if (err) break;
    *p = '/';
    l = p+1;
    p = strchr(l, '/');
  }
  if (err) return nerr_pass(err);
  if (strlen(l))
  {
    snprintf(buf, sizeof(buf), "%s.%d", prefix, n);
    err = hdf_set_value(cgi->hdf, buf, l);
    if (err) return nerr_pass(err);
    snprintf(buf, sizeof(buf), "%s.%d.path", prefix, n++);
    err = hdf_set_value(cgi->hdf, buf, album);
    if (err) return nerr_pass(err);
  }

  return STATUS_OK;
}


NEOERR *dowork_picture (CGI *cgi, char *album, char *picture)
{
  NEOERR *err = STATUS_OK;
  char *base, *name;
  char path[_POSIX_PATH_MAX];
  char buf[256];
  int i, x, factor, y;
  int thumb_width, thumb_height;
  int pic_width, pic_height;
  ULIST *files = NULL;
  char t_album[_POSIX_PATH_MAX];
  char t_pic[_POSIX_PATH_MAX];
  char nfile[_POSIX_PATH_MAX];
  char *ch;
  char *avi = NULL;
  int rotate;

  ch = strrchr(picture, '/');
  if (ch != NULL)
  {
    *ch = '\0';
    snprintf(t_album, sizeof(t_album), "%s/%s", album, picture);
    *ch = '/';
    strncpy(t_pic, ch+1, sizeof(t_pic));
    picture = t_pic;
    album = t_album;
  }

  base = hdf_get_value (cgi->hdf, "BASEDIR", NULL);
  if (base == NULL)
  {
    cgi_error (cgi, "No BASEDIR in imd file");
    return nerr_raise(CGIFinished, "Finished");
  }

  thumb_width = hdf_get_int_value (cgi->hdf, "ThumbWidth", 120);
  thumb_height = hdf_get_int_value (cgi->hdf, "ThumbWidth", 90);
  pic_width = hdf_get_int_value (cgi->hdf, "PictureWidth", 120);
  pic_height = hdf_get_int_value (cgi->hdf, "PictureWidth", 90);

  err = hdf_set_value (cgi->hdf, "Context", "picture");
  if (err != STATUS_OK) return nerr_pass(err);

  snprintf (path, sizeof(path), "%s/%s", base, album);
  rotate = hdf_get_int_value(cgi->hdf, "Query.rotate", 0);
  if (rotate)
  {
    err = rotate_image(path, picture, rotate, nfile);
    if (err) return nerr_pass(err);
    picture = strrchr(nfile, '/') + 1;
  }

  err = hdf_set_value (cgi->hdf, "Album", album);
  if (err != STATUS_OK) return nerr_pass(err);
  err = hdf_set_value (cgi->hdf, "Album.Raw", album);
  if (err != STATUS_OK) return nerr_pass(err);
  err = export_album_path(cgi, album, "Album.Path");
  if (err) return nerr_pass(err);
  err = hdf_set_value (cgi->hdf, "Picture", picture);
  if (err != STATUS_OK) return nerr_pass(err);

  err = load_images(path, &files, NULL, 0);
  if (err != STATUS_OK) return nerr_pass(err);
  err = uListSort(files, alpha_sort);
  if (err != STATUS_OK) return nerr_pass(err);

  i = -1;
  for (x = 0; x < uListLength(files); x++)
  {
    err = uListGet(files, x, (void *)&name);
    if (err) break;
    if (!strcmp(name, picture))
    {
      i = x;
      break;
    }
  }
  if (i != -1)
  {
    for (x = 2; x > 0; x--)
    {
      if (i - x < 0) continue;
      err = uListGet(files, i-x, (void *)&name);
      if (err) break;
      snprintf(buf, sizeof(buf), "Show.%d", i-x);
      err = export_image(cgi, buf, path, name);
      if (err) break;
    }
    for (x = 0; x < 3; x++)
    {
      if (i + x > uListLength(files)) break;
      err = uListGet(files, i+x, (void *)&name);
      if (err) break;
      snprintf(buf, sizeof(buf), "Show.%d", i+x);
      err = export_image(cgi, buf, path, name);
      if (err) break;
    }
    snprintf (buf, sizeof(buf), "Show.%d.width", i);
    x = hdf_get_int_value (cgi->hdf, buf, -1);
    if (x != -1)
    {
      factor = 1;
      y = x;
      while (y > pic_width)
      {
	factor = factor * 2;
	/* factor++; */
	y = x / factor;
	ne_warn("factor = %d, y = %d", factor, y);
      }
      snprintf (buf, sizeof(buf), "%d", y);
      hdf_set_value (cgi->hdf, "Picture.width", buf);
      snprintf (buf, sizeof(buf), "Show.%d.height", i);
      x = hdf_get_int_value (cgi->hdf, buf, -1);
      y = x / factor;
      snprintf (buf, sizeof(buf), "%d", y);
      hdf_set_value (cgi->hdf, "Picture.height", buf);
    }
    else
    {
      snprintf (buf, sizeof(buf), "%d", pic_width);
      hdf_set_value (cgi->hdf, "Picture.width", buf);
      snprintf (buf, sizeof(buf), "%d", pic_height);
      hdf_set_value (cgi->hdf, "Picture.height", buf);
    }
    snprintf (buf, sizeof(buf), "Show.%d.avi", i);
    avi = hdf_get_value (cgi->hdf, buf, NULL);
    if (avi) 
    {
      err = hdf_set_value(cgi->hdf, "Picture.avi", avi);
    }

    err = scale_images (cgi, "Show", thumb_width, thumb_height, 0);
  }
  uListDestroy(&files, ULIST_FREE);

  return nerr_pass(err);
}

static int is_album(void *rock, char *filename)
{
  char path[_POSIX_PATH_MAX];
  char *prefix = (char *)rock;

  if (filename[0] == '.') return 0;
  snprintf(path, sizeof(path), "%s/%s", prefix, filename);
  if (isdir(path)) return 1;
  return 0;
}

NEOERR *dowork_album_overview (CGI *cgi, char *album)
{
  NEOERR *err = STATUS_OK;
  DIR *dp;
  struct dirent *de;
  char path[_POSIX_PATH_MAX];
  char buf[256];
  int i = 0, x, y;
  int thumb_width, thumb_height;
  ULIST *files = NULL;
  ULIST *albums = NULL;
  char *name;

  thumb_width = hdf_get_int_value (cgi->hdf, "ThumbWidth", 120);
  thumb_height = hdf_get_int_value (cgi->hdf, "ThumbWidth", 90);

  err = ne_listdir_fmatch(album, &albums, is_album, album);
  if (err) return nerr_pass(err);


  err = uListSort(albums, alpha_sort);
  if (err) return nerr_pass(err);
  for (y = 0; y < uListLength(albums); y++)
  {
    err = uListGet(albums, y, (void *)&name);
    if (err) break;

    snprintf(path, sizeof(path), "%s/%s", album, name);
    snprintf(buf, sizeof(buf), "Albums.%d", i);
    err = hdf_set_value (cgi->hdf, buf, name);
    if (err != STATUS_OK) break;
    err = load_images(path, &files, NULL, 1);
    if (err != STATUS_OK) break;
    err = uListSort(files, alpha_sort);
    if (err != STATUS_OK) break;
    snprintf(buf, sizeof(buf), "Albums.%d.Count", i);
    err = hdf_set_int_value(cgi->hdf, buf, uListLength(files));
    if (err != STATUS_OK) break;
    for (x = 0; (x < 4) && (x < uListLength(files)); x++)
    {
      err = uListGet(files, x, (void *)&name);
      if (err) break;
      snprintf(buf, sizeof(buf), "Albums.%d.Images.%d", i, x);
      err = export_image(cgi, buf, path, name);
      if (err) break;
    }
    uListDestroy(&files, ULIST_FREE);
    if (err != STATUS_OK) break;
    snprintf(buf, sizeof(buf), "Albums.%d.Images", i);
    err = scale_images (cgi, buf, thumb_width, thumb_height, 0);
    if (err != STATUS_OK) break;
    i++;
  }
  return nerr_pass(err);
}

NEOERR *dowork_album (CGI *cgi, char *album)
{
  NEOERR *err;
  char *base;
  char buf[256];
  char path[_POSIX_PATH_MAX];
  int thumb_width, thumb_height;
  int per_page, start, next, prev, last;
  ULIST *files = NULL;
  char *name;
  int x;

  base = hdf_get_value (cgi->hdf, "BASEDIR", NULL);
  if (base == NULL)
  {
    cgi_error (cgi, "No BASEDIR in imd file");
    return nerr_raise(CGIFinished, "Finished");
  }
  thumb_width = hdf_get_int_value (cgi->hdf, "ThumbWidth", 120);
  thumb_height = hdf_get_int_value (cgi->hdf, "ThumbWidth", 90);
  per_page = hdf_get_int_value (cgi->hdf, "PerPage", 50);
  start = hdf_get_int_value (cgi->hdf, "Query.start", 0);

  err = hdf_set_value (cgi->hdf, "Album", album);
  if (err != STATUS_OK) return nerr_pass(err);
  err = hdf_set_value (cgi->hdf, "Album.Raw", album);
  if (err != STATUS_OK) return nerr_pass(err);
  err = export_album_path(cgi, album, "Album.Path");
  if (err) return nerr_pass(err);


  err = hdf_set_value (cgi->hdf, "Context", "album");
  if (err != STATUS_OK) return nerr_pass(err);


  snprintf (path, sizeof(path), "%s/%s", base, album);
  err = dowork_album_overview(cgi, path);
  if (err != STATUS_OK) return nerr_pass(err);
  
  err = load_images(path, &files, NULL, 0);
  if (err != STATUS_OK) return nerr_pass (err);
  err = uListSort(files, alpha_sort);
  if (err != STATUS_OK) return nerr_pass (err);
  err = hdf_set_int_value(cgi->hdf, "Album.Count", uListLength(files));
  if (err != STATUS_OK) return nerr_pass (err);
  if (start > uListLength(files)) start = 0;
  next = start + per_page;
  if (next > uListLength(files)) next = uListLength(files);
  prev = start - per_page;
  if (prev < 0) prev = 0;
  last = uListLength(files) - per_page;
  if (last < 0) last = 0;
  err = hdf_set_int_value(cgi->hdf, "Album.Start", start);
  if (err != STATUS_OK) return nerr_pass (err);
  err = hdf_set_int_value(cgi->hdf, "Album.Next", next);
  if (err != STATUS_OK) return nerr_pass (err);
  err = hdf_set_int_value(cgi->hdf, "Album.Prev", prev);
  if (err != STATUS_OK) return nerr_pass (err);
  err = hdf_set_int_value(cgi->hdf, "Album.Last", last);
  if (err != STATUS_OK) return nerr_pass (err);
  for (x = start; x < next; x++)
  {
    err = uListGet(files, x, (void *)&name);
    if (err) break;
    snprintf(buf, sizeof(buf), "Images.%d", x);
    err = export_image(cgi, buf, path, name);
    if (err) break;
  }
  uListDestroy(&files, ULIST_FREE);
  if (err != STATUS_OK) return nerr_pass (err);
  err = scale_images (cgi, "Images", thumb_width, thumb_height, 0);
  if (err != STATUS_OK) return nerr_pass (err);
  return STATUS_OK;
}

NEOERR *dowork_image (CGI *cgi, char *image) 
{
  NEOERR *err = STATUS_OK;
  int maxW = 0, maxH = 0;
  char *basepath = "";
  char *cache_basepath = "/tmp/.imgcache/";
  char srcpath[_POSIX_PATH_MAX] = "";
  char cachepath[_POSIX_PATH_MAX] = "";
  char buf[256];
  char *if_mod;
  int i, l, quality;
  struct stat s;
  struct tm *t;

  if ((i = hdf_get_int_value(cgi->hdf, "Query.width", 0)) != 0) {
    maxW = i;
  }

  if ((i = hdf_get_int_value(cgi->hdf, "Query.height", 0)) != 0) {
    maxH = i;
  }
  quality = hdf_get_int_value(cgi->hdf, "Query.quality", 0);

  if_mod = hdf_get_value(cgi->hdf, "HTTP.IfModifiedSince", NULL);

  basepath = hdf_get_value(cgi->hdf, "BASEDIR", NULL);
  if (basepath == NULL)
  {
    cgi_error (cgi, "No BASEDIR in imd file");
    return nerr_raise(CGIFinished, "Finished");
  }

  snprintf (srcpath, sizeof(srcpath), "%s/%s", basepath, image);
  snprintf (cachepath, sizeof(cachepath), "%s/%dx%d/%s", cache_basepath, 
      maxW, maxH,image);

  if (stat(srcpath, &s))
  {
    cgiwrap_writef("Status: 404\nContent-Type: text/html\n\n");
    cgiwrap_writef("File %s not found.", srcpath);
    return nerr_raise_errno(NERR_IO, "Unable to stat file %s", srcpath);
  }

  t = gmtime(&(s.st_mtime));
  if (if_mod && later_than(t, if_mod))
  {
    cgiwrap_writef("Status: 304\nContent-Type: text/html\n\n");
    cgiwrap_writef("Use Local Copy");
    return STATUS_OK;
  }

  /* fprintf(stderr,"cachepath: %s\n",cachepath); */

  ne_warn("srcpath: %s", srcpath);
  l = strlen(srcpath);
  if ((l>4 && !strcasecmp(srcpath+l-4, ".jpg")) ||
      (l>4 && !strcasecmp(srcpath+l-4, ".thm")) ||
      (l>5 && !strcasecmp(srcpath+l-5, ".jpeg")))
    cgiwrap_writef("Content-Type: image/jpeg\n");
  else if (l>4 && !strcasecmp(srcpath+l-4, ".gif"))
    cgiwrap_writef("Content-Type: image/gif\n");
  else if (l>4 && !strcasecmp(srcpath+l-4, ".avi"))
  {
    ne_warn("found avi");
    cgiwrap_writef("Content-Type: video/x-msvideo\n");
  }
  t = gmtime(&(s.st_mtime));
  strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", t);
  cgiwrap_writef("Last-modified: %s\n", buf);

  err = scale_and_display_image(srcpath,maxW,maxH,cachepath,quality);
  return nerr_pass(err);
}

int main(int argc, char **argv, char **envp)
{
  NEOERR *err;
  CGI *cgi;
  char *image;
  char *album;
  char *imd_file;
  char *cs_file;
  char *picture;

  ne_warn("Starting IMD");
  cgi_debug_init (argc,argv);
  cgiwrap_init_std (argc, argv, envp);
  
  nerr_init();

  ne_warn("CGI init");
  err = cgi_init(&cgi, NULL);
  if (err != STATUS_OK)
  {
    nerr_log_error(err);
    cgi_destroy(&cgi);
    return -1;
  }
  imd_file = hdf_get_value(cgi->hdf, "CGI.PathTranslated", NULL);
  ne_warn("Reading IMD file %s", imd_file);
  err = hdf_read_file (cgi->hdf, imd_file);
  if (err != STATUS_OK)
  {
    cgi_neo_error(cgi, err);
    nerr_log_error(err);
    cgi_destroy(&cgi);
    return -1;
  }

  cs_file = hdf_get_value(cgi->hdf, "Template", NULL);
  image = hdf_get_value(cgi->hdf, "Query.image", NULL);
  album = hdf_get_value(cgi->hdf, "Query.album", "");
  picture = hdf_get_value(cgi->hdf, "Query.picture", NULL);
  if (image)
  {
    err = dowork_image(cgi, image);
    if (err)
    {
      nerr_log_error(err);
      cgi_destroy(&cgi);
      return -1;
    }
  }
  else 
  {
    if (!picture)
    {
      err = dowork_album (cgi, album);
    }
    else
    {
      err = dowork_picture (cgi, album, picture);
    }
    if (err != STATUS_OK)
    {
      if (nerr_handle(&err, CGIFinished))
      {
	/* pass */
      }
      else
      {
	cgi_neo_error(cgi, err);
	nerr_log_error(err);
        cgi_destroy(&cgi);
	return -1;
      }
    }
    else
    {
      err = cgi_display(cgi, cs_file);
      if (err != STATUS_OK)
      {
	cgi_neo_error(cgi, err);
	nerr_log_error(err);
        cgi_destroy(&cgi);
	return -1;
      }
    }
  }
  cgi_destroy(&cgi);
  return 0;
}
