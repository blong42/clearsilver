<?cs def:frame_picture(image, link, size) ?>
    <TABLE cellspacing=0 cellpadding=0 border=0 WIDTH=1%>
      <TR>
	<TD><IMG name="frame0" border=0 height=8 width=8 src="0.gif"></TD>
	<TD><IMG name="frame1" border=0 height=8 width=<?cs var:image.width ?> src="1.gif"></TD>
	<TD><IMG name="frame2" border=0 height=8 width=8 src="2.gif"></TD>
      </TR>
      <TR>
	<TD><IMG name="frame3" border=0 height=<?cs var:image.height ?> width=8 src="3.gif"></TD>
	<TD><a href="<?cs var:link ?>"><img border=0 width=<?cs var:image.width ?> height=<?cs var:image.height ?> src="<?cs var:CGI.PathInfo?>?image=<?cs var:Album ?>/<?cs var:image ?>&width=<?cs var:image.width ?>&height=<?cs var:image.height ?>"></a></TD>
	<TD><IMG name="frame4" border=0 height=<?cs var:image.height ?> width=8 src="4.gif"></TD>
      </TR>
      <TR>
	<TD><IMG name="frame5" border=0 height=8 width=8 src="5.gif"></TD>
	<TD><IMG name="frame6" border=0 height=8 width=<?cs var:image.width ?> src="6.gif"></TD>
	<TD><IMG name="frame7" border=0 height=8 width=8 src="7.gif"></TD>
      </TR>
    </TABLE>
<?cs /def ?>
<HTML>
<HEAD>
<TITLE><?cs var:Title ?><?cs if:Context == "album" ?> <?cs var:Album.Raw ?> <?cs var:Album.Start + #1 ?> - <?cs var:Album.Next ?> of <?cs var:Album.Count ?><?cs else ?> <?cs var:Album.Raw ?>
- <?cs var:Picture ?><?cs /if ?></TITLE>
</HEAD>
<BODY BGCOLOR=#ffffff>
  <?cs if:Context == "album" ?>
    <table border=0 bgcolor=#cccccc width=100%>
      <tr><td align=center><font size=+2><?cs var:Album.Raw ?></font></td></tr>
    </table>
  <?cs if:Albums.0 ?>
    <H1><?cs var:Title ?></H1>
    <CENTER>
      <?cs each:album = Albums ?>
	<TABLE BORDER=0 CELLSPACING=1 CELLPADDING=1>
	  <TR><TD BGCOLOR=#cccccc COLSPAN=4><font size=+2>
	    <a href="<?cs var:CGI.PathInfo?>?album=<?cs if:Album ?><?cs var:Album ?>/<?cs /if ?><?cs var:album ?>"><?cs var:album ?></a></font> (<?cs var:album.Count ?> images)
	  </td></tr>
	  <TR>
		  <?cs each:image = album.Images ?>
		    <td align=center>
    <TABLE cellspacing=0 cellpadding=0 border=0 WIDTH=1%>
      <TR>
	<TD><IMG name="frame0" border=0 height=8 width=8 src="0.gif"></TD>
	<TD><IMG name="frame1" border=0 height=8 width=<?cs var:image.width ?> src="1.gif"></TD>
	<TD><IMG name="frame2" border=0 height=8 width=8 src="2.gif"></TD>
      </TR>
      <TR>
	<TD><IMG name="frame3" border=0 height=<?cs var:image.height ?> width=8 src="3.gif"></TD>
	<TD><a href="<?cs var:CGI.PathInfo?>?album=<?cs if:Album ?><?cs var:Album ?>/<?cs /if ?><?cs var:album ?>&picture=<?cs var:image ?>"><img border=0 width=<?cs var:image.width ?> height=<?cs var:image.height ?> src="<?cs var:CGI.PathInfo?>?image=<?cs if:Album ?><?cs var:Album ?>/<?cs /if ?><?cs var:album ?>/<?cs var:image ?>&width=<?cs var:image.width ?>&height=<?cs var:image.height ?>"></a></TD>
	<TD><IMG name="frame4" border=0 height=<?cs var:image.height ?> width=8 src="4.gif"></TD>
      </TR>
      <TR>
	<TD><IMG name="frame5" border=0 height=8 width=8 src="5.gif"></TD>
	<TD><IMG name="frame6" border=0 height=8 width=<?cs var:image.width ?> src="6.gif"></TD>
	<TD><IMG name="frame7" border=0 height=8 width=8 src="7.gif"></TD>
      </TR>
    </TABLE>
		    </td>
		  <?cs /each ?>
	  </TR>
	</TABLE>
      <?cs /each ?>
    </CENTER>
    <?cs /if ?>
    <?cs if:#Album.Count ?>
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
      <tr><td align=center><font size=+2><?cs var:Album.Raw ?></font></td></tr>
    </table>
    <TABLE>
      <TR>
       <?cs set:TotalWidth = #0 ?>
    <?cs each:image=Images ?>
      <?cs set:nextWidth = #TotalWidth + #image.width ?>
      <?cs if:#nextWidth > #PageWidth ?></TR></TABLE><TABLE><TR><?cs set:TotalWidth = image.width ?><?cs else ?><?cs set:TotalWidth = nextWidth ?><?cs /if ?>
      <TD><?cs call:frame_picture(image, CGI.PathInfo + "?album=" + Album + "&picture=" + image, "8") ?></TD>
    <?cs /each ?>
      </TR>
    </TABLE>
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
  <?cs /if ?>
  <?cs else ?><?cs # picture ?>
    <A HREF="<?cs var:CGI.PathInfo?>"><?cs var:Title ?></A> 
    -- <A HREF="<?cs var:CGI.PathInfo?>?album=<?cs var:Album ?>"><?cs var:Album.Raw ?></A>
    <DIV ALIGN=RIGHT>
      <?cs set:count = #0 ?>
      <?cs each:image=Show ?>
	<a href="<?cs var:CGI.PathInfo?>?album=<?cs var:Album ?>&picture=<?cs var:image ?>">
	 <?cs if:count == #0 ?>
	   -2
	 <?cs elif:count == #1 ?>
	   -1
	 <?cs elif:count == #2 ?>
	    <?cs Name:image ?>
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
    <TABLE cellspacing=0 cellpadding=0 border=0 align=center>
      <TR>
        <TD><IMG name="frame0" border=0 height=18 width=18 src="0.gif"></TD>
        <TD><IMG name="frame1" border=0 height=18 width=<?cs var:Picture.width ?> src="1.gif"></TD>
        <TD><IMG name="frame2" border=0 height=18 width=18 src="2.gif"></TD>
      </TR>
      <TR>
        <TD><IMG name="frame3" border=0 height=<?cs var:Picture.height ?> width=18 src="3.gif"></TD>
	<TD><img border=0 width=<?cs var:Picture.width?> height=<?cs var:Picture.height?> src="<?cs var:CGI.PathInfo?>?image=<?cs var:Album ?>/<?cs var:Picture ?>&width=<?cs var:Picture.width ?>&height=<?cs var:Picture.height?>&quality=1"></TD>
        <TD><IMG name="frame4" border=0 height=<?cs var:Picture.height ?> width=18 src="4.gif"></TD>
      </TR>
      <TR>
        <TD><IMG name="frame5" border=0 height=18 width=18 src="5.gif"></TD>
        <TD><IMG name="frame6" border=0 height=18 width=<?cs var:Picture.width ?> src="6.gif"></TD>
        <TD><IMG name="frame7" border=0 height=18 width=18 src="7.gif"></TD>
      </TR>
    </TABLE>
    <CENTER>
      <TABLE>
	<TR>
	  <?cs each:image=Show ?>
	  <?cs if:image != Picture ?>
	    <TD><?cs call:frame_picture(image, CGI.PathInfo + "?album=" + Album + "&picture=" + image, "8") ?></TD>
	  <?cs /if ?>
	  <?cs /each ?>
	</TR>
      </TABLE>
    </CENTER>
  <?cs /if ?>
</BODY>
</HTML>
