#!/usr/local/bin/python
#
# imdm
# 
# IMage Display Master
#
# This uses the affiliated C-cgi "imd" to build a caching image display
# server with only passive Apache cgis.
#
import sys,os,string
import cgi

# this function should find the first four images inside a
# nested subdirectory

albumstartfile = "/~jeske/Images/jeskealbum.imd"
imagestartfile = "/~jeske/Images/jeskealbum.imc"

THUMB_WIDTH = 120
THUMB_HEIGHT = 90 

# ------------------------------------------------------------------------------------
#
# utility functions

def albumoverview(basedir,sub_dir,count = 4,skip = 0):
    images = []

    fulldir = os.path.join(basedir,sub_dir)
    for a_entry in os.listdir(fulldir):
	fullpath = os.path.join(fulldir,a_entry)
	if os.path.isfile(fullpath):
	    if string.lower(string.split(a_entry,".")[-1]) in ["jpeg","jpg"]:
		images.append(os.path.join(sub_dir,a_entry))
	elif os.path.isdir(fullpath):
	    images + albumoverview(basedir,os.path.join(sub_dir,a_entry),1)
	if len(images) >= (count + skip):
	    return images[skip:]

    return images[skip:]


def makethumbnailimgtag(filename,width=THUMB_WIDTH,height=THUMB_HEIGHT):
    global imagestartfile
    return '<IMG SRC="%s?image=%s&width=%s&height=%s">' % (imagestartfile,filename,width,height)

def makealbumurl(dir):
    global albumstartfile
    return "%s?album=%s" % (albumstartfile,dir)

def makepictureurl(dir,picture):
    global albumstartfile
    return "%s?album=%s&picture=%s" % (albumstartfile,dir,picture)

# ------------------------------------------------------------------------------------
#
# picturedisplay

def picturedisplay(basedir,album,picture):
    sys.stdout.write("<A HREF=\"%s?\">top</A> " % albumstartfile)

    sys.stdout.write("-- <A HREF=\"%s\">%s</A>" % (makealbumurl(album),album))

    imagename = os.path.join(album,picture)

    sys.stdout.write("<br><hr>")

    sys.stdout.write("<TABLE WIDTH=100%><TR><TD ALIGN=CENTER>\n")
    sys.stdout.write(makethumbnailimgtag(imagename,width=600,height=500))
    sys.stdout.write("</TD></TR></TABLE>\n")

    images = albumoverview(basedir,album,count=500)
    image_index = None
    for x in range(len(images)):
	if images[x] == imagename:
	    image_index = x
	    break

    if not image_index is None:
	sys.stdout.write("<CENTER><TABLE WIDTH=50% BORDER=1><TR>")
	
	# pre-images
	for i in range(1,3):
	    pic_index = image_index - i
	    
	    picture_path = string.join(string.split(images[pic_index],'/')[1:],'/')
	    sys.stdout.write("<TD ALIGN=CENTER><A HREF=\"%s\">%s</A></TD>" % (makepictureurl(album,picture_path),makethumbnailimgtag(images[pic_index])))

	sys.stdout.write("<br>")

	# post-images
	for i in range(1,3):
	    pic_index = image_index + i
	    if pic_index >= len(images):
		pic_index = pic_index - len(images)
	    
	    picture_path = string.join(string.split(images[pic_index],'/')[1:],'/')
	    sys.stdout.write("<TD ALIGN=CENTER><A HREF=\"%s\">%s</A></TD>" % (makepictureurl(album,picture_path), makethumbnailimgtag(images[pic_index])))
	sys.stdout.write("</TR></TABLE></CENTER>\n")
	    
    # navigation


# ------------------------------------------------------------------------------------
#
#  albumdisplay

    

def albumdisplay(basedir,album,columns=7,rows=5):
    next_page = 0

    
    sys.stdout.write("<table border=0 bgcolor=#cccccc width=100%%><tr><td align=center><font size=+2>%s</font></td></tr></table>" % album)

    imgcount = columns * rows 
    images = albumoverview(basedir,album,count=(imgcount + 1),skip=0)

    if len(images) > imgcount:
	images = images[:-1]
	next_page = 1

    while images:
	sys.stdout.write("<CENTER><TABLE WIDTH=90% CELLSPACING=3 BORDER=1><TR>")
	for a_col in range(columns):
	    if len(images):
		picture_path = string.join(string.split(images[0],'/')[1:],'/')
		sys.stdout.write("<TD ALIGN=CENTER><A HREF=\"%s\">%s</A></TD>" % (makepictureurl(album,picture_path),makethumbnailimgtag(images[0])))
		images = images[1:]
	sys.stdout.write("</tr></table></CENTER>")

    if next_page:
	sys.stdout.write("more...")
    

# ------------------------------------------------------------------------------------
#
# topalbumoverview

def topalbumoverview(dir):
    for a_dir in os.listdir(dir):
	if os.path.isdir(os.path.join(dir,a_dir)):
	    sys.stdout.write("<CENTER>")
	    sys.stdout.write("<TABLE BGCOLOR=#ccccc WIDTH=50% BORDER=0 CELLSPACING=1 CELLPADDING=1>")
	    
	    sys.stdout.write("<TR><TD> <font size=+2><A HREF=\"%s\">%s</A></font></TD></TR>" % (makealbumurl(a_dir),a_dir))

	    sys.stdout.write("<TR><TD ALIGN=CENTER><TABLE BGCOLOR=#FFFFFF WIDTH=100% BORDER=0 CELSPACING=0 CELLPADDING=0><TR>")
			     
	    for a_file in albumoverview(dir,a_dir):
		picture_path = string.join(string.split(a_file,'/')[1:],'/')
		sys.stdout.write("<TD ALIGN=CENTER><A HREF=\"%s\">%s</A></TD>\n" % (makepictureurl(a_dir,picture_path),makethumbnailimgtag(a_file)))
	    sys.stdout.write("</TR></TABLE></TD></TR></TABLE></CENTER>\n<p>\n")

# ------------------------------------------------------------------------------------
#
# readvars() -- simple file format reader

def readvars(filename):
    vars = {}
    data = open(filename,"rb").read()
    lines = string.split(data,"\n")
    for a_line in lines:
	stripped_line = string.strip(a_line)
	if not stripped_line or stripped_line[0] == "#":
	    continue
	try:
	    key,value = string.split(a_line,"=")
	    vars[key] = value
	except:
	    pass
    return vars

# ------------------------------------------------------------------------------------
#
# main()


def main():
    global cgiform
    cgiform = cgi.FieldStorage()

    sys.stdout.write("Content-Type: text/html\n\n")
    sys.stdout.write("<h1>HTML Image Viewer!</h1><p>")

    myvars = readvars(os.environ['PATH_TRANSLATED'])

    sys.stderr.write(repr(myvars))
    
    global albumstartfile, imagestartfile
    albumstartfile = os.environ['PATH_INFO']
    imagestartfile = myvars['IMGSTARTFILE']
    BASEDIR        = myvars['BASEDIR']

    album = cgiform.getvalue('album',None)
    picture = cgiform.getvalue('picture',None)

    if album is None:
	topalbumoverview(BASEDIR)
    elif picture is None:
	albumdisplay(BASEDIR,album)
    else:
	picturedisplay(BASEDIR,album,picture)

if __name__ == "__main__":
    main()



