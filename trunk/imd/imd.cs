<HTML>
<HEAD>
<TITLE><?cs var:Title ?></TITLE>
</HEAD>
<BODY>
  <?cs if:Context == "overview" ?>
    <H1>Albums</H1>
    <CENTER>
      <?cs each:album = Albums ?>
	<TABLE BGCOLOR=#cccccc WIDTH=50% BORDER=0 CELLSPACING=1 CELLPADDING=1>
	  <TR><TD><font size=+2>
	    <a href="<?cs var:CGI.PathInfo?>?album=<?cs var:album ?>"><?cs var:album ?></a></font>
	  </td></tr>
	  <TR>
	    <TD ALIGN=CENTER>
	      <TABLE BGCOLOR=#FFFFFF WIDTH=100% BORDER=0 CELSPACING=0 CELLPADDING=0>
	        <TR>
		  <?cs each:image = album.Images ?>
		    <td align=center>
		      <a href="<?cs var:CGI.PathInfo?>?album=<?cs var:album ?>&picture=<?cs var:image ?>"><img width=<?cs var:image.width ?> height=<?cs var:image.height ?> src="<?cs var:CGI.PathInfo?>?image=<?cs var:album ?>/<?cs var:image ?>&width=<?cs var:image.width ?>&height=<?cs var:image.height ?>"></a>
		    </td>
		  <?cs /each ?>
		</TR>
              </TABLE>
	    </TD>
	  </TR>
	</TABLE>
      <?cs /each ?>
    </CENTER>
  <?cs elif:Context == "album" ?>
  <A HREF="<?cs var:CGI.PathInfo?>">top</A> 
    <table border=0 bgcolor=#cccccc width=100%>
      <tr><td align=center><font size=+2><?cs var:Album ?></font></td></tr>
    </table>
    <?cs each:image=Images ?>
      <a href="<?cs var:CGI.PathInfo?>?album=<?cs var:Album ?>&picture=<?cs var:image ?>"><img width=<?cs var:image.width ?> height=<?cs var:image.height ?> src="<?cs var:CGI.PathInfo?>?image=<?cs var:Album ?>/<?cs var:image ?>&width=<?cs var:image.width ?>&height=<?cs var:image.height ?>"></a>
    <?cs /each ?>
  <?cs else ?>
  <A HREF="<?cs var:CGI.PathInfo?>">top</A> 
  -- <A HREF="<?cs var:CGI.PathInfo?>?album=<?cs var:Album ?>"><?cs var:Album ?></A>
  <br><hr>
  <TABLE WIDTH=100%>
    <TR>
      <TD ALIGN=CENTER>
	<img width=<?cs var:Picture.width?> height=<?cs var:Picture.height?> src="<?cs var:CGI.PathInfo?>?image=<?cs var:Album ?>/<?cs var:Picture ?>&width=<?cs var:Picture.width ?>&height=<?cs var:Picture.height?>&quality=1"></a>
      </TD>
    </TR>
  </TABLE>
  <CENTER>
    <TABLE WIDTH=50% BORDER=1>
      <TR>
        <?cs each:image=Show ?>
        <TD ALIGN=CENTER>
	  <a href="<?cs var:CGI.PathInfo?>?album=<?cs var:Album ?>&picture=<?cs var:image ?>"><img width=<?cs var:image.width ?> height=<?cs var:image.height ?> src="<?cs var:CGI.PathInfo?>?image=<?cs var:Album ?>/<?cs var:image ?>&width=<?cs var:image.width ?>&height=<?cs var:image.height ?>"></a>
	</TD>
	<?cs /each ?>
    </TABLE>
  </CENTER>
  <?cs /if ?>
</BODY>
</HTML>
