<HTML>
<HEAD>
<TITLE><?cs var:Title ?><?cs if:Context == "overview" ?><?cs 
elif:Context == "album" ?> <?cs var:Album ?> <?cs var:Album.Start + #1
?> - <?cs var:Album.Next ?> of <?cs var:Album.Count ?><?cs else ?> <?cs
var:Album ?> - <?cs var:Picture ?><?cs /if ?></TITLE>
</HEAD>
<BODY BGCOLOR=#ffffff>
  <?cs if:Context == "overview" ?>
    <H1>Albums</H1>
    <CENTER>
      <?cs each:album = Albums ?>
	<TABLE BGCOLOR=#cccccc WIDTH=50% BORDER=0 CELLSPACING=1 CELLPADDING=1>
	  <TR><TD><font size=+2>
	    <a href="<?cs var:CGI.PathInfo?>?album=<?cs var:album ?>"><?cs var:album ?></a></font> (<?cs var:album.Count ?> images)
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
    <A HREF="<?cs var:CGI.PathInfo?>"><?cs var:Title ?></A> 
    <DIV ALIGN=RIGHT>
    <?cs if:Album.Start > #0 ?>
    <A HREF="<?cs var:CGI.PathInfo ?>?album=<?cs var:Album ?>">First</A>
    <?cs else ?>
    First
    <?cs /if ?>
    &nbsp;
    <?cs if:Album.Prev > #0 ?>
    <A HREF="<?cs var:CGI.PathInfo ?>?album=<?cs var:Album ?>&start=<?cs var:Album.Prev ?>">Prev</A>
    <?cs else ?>
    Prev
    <?cs /if ?>
    &nbsp;
    <?cs var:#Album.Start + #1 ?> - <?cs var:#Album.Next ?> of <?cs var:#Album.Count ?>
    &nbsp;
    <?cs if:#Album.Next < #Album.Count ?>
    <A HREF="<?cs var:CGI.PathInfo ?>?album=<?cs var:Album ?>&start=<?cs var:Album.Next ?>">Next</A>
    <?cs else ?>
    Next
    <?cs /if ?>
    &nbsp;
    <?cs if:#Album.Start < #Album.Last ?>
    <A HREF="<?cs var:CGI.PathInfo ?>?album=<?cs var:Album ?>&start=<?cs var:Album.Last ?>">Last</A>
    <?cs else ?>
    Last
    <?cs /if ?>
    </DIV>
    
    <table border=0 bgcolor=#cccccc width=100%>
      <tr><td align=center><font size=+2><?cs var:Album ?></font></td></tr>
    </table>
    <?cs each:image=Images ?>
      <a href="<?cs var:CGI.PathInfo?>?album=<?cs var:Album ?>&picture=<?cs var:image ?>"><img width=<?cs var:image.width ?> height=<?cs var:image.height ?> src="<?cs var:CGI.PathInfo?>?image=<?cs var:Album ?>/<?cs var:image ?>&width=<?cs var:image.width ?>&height=<?cs var:image.height ?>"></a>
    <?cs /each ?>
  <?cs else ?><?cs # picture ?>
    <A HREF="<?cs var:CGI.PathInfo?>"><?cs var:Title ?></A> 
    -- <A HREF="<?cs var:CGI.PathInfo?>?album=<?cs var:Album ?>"><?cs var:Album ?></A>
    <DIV ALIGN=RIGHT>
      <?cs set:count = #0 ?>
      <?cs each:image=Show ?>
	<a href="<?cs var:CGI.PathInfo?>?album=<?cs var:Album ?>&picture=<?cs var:image ?>">
	 <?cs if:count == #0 ?>
	   -2
	 <?cs elif:count == #1 ?>
	   -1
	 <?cs elif:count == #2 ?>
	    0
	 <?cs elif:count == #3 ?>
	    1
	 <?cs elif:count == #4 ?>
	   +2
	 <?cs /if ?>
	 <?cs set:count = count + #1 ?>
	</a> &nbsp;
      <?cs /each ?>
    </DIV>
    <hr>
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
