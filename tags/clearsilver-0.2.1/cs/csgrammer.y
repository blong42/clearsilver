
file ::= contents.

contents ::= LITERAL contents.
contents ::= command contents.
contents ::= LITERAL.
contents ::= command.

command ::= CS_OPEN cmd CS_CLOSE.
command	::= if_cmd.
command	::= each_cmd.
command	::= def_cmd.

cmd ::= VAR opt_req expr.
cmd ::= EVAR opt_req expr.
cmd ::= INCLUDE opt_req expr.
cmd ::= SET opt_req expr EQUALS expr.
cmd ::= CALL opt_req expr LPAREN comma_exprs RPAREN.

if_cmd ::= CS_OPEN IF opt_req expr CS_CLOSE else_contents CS_OPEN ENDIF CS_CLOSE.
each_cmd ::= CS_OPEN EACH opt_req expr EQUALS expr CS_CLOSE contents CS_OPEN ENDEACH CS_CLOSE.
def_cmd ::= CS_OPEN DEF opt_req expr LPAREN comma_exprs RPAREN CS_CLOSE contents CS_OPEN ENDDEF CS_CLOSE.

else_contents ::= contents else_contents.
else_contents ::= CS_OPEN ELIF opt_req expr CS_CLOSE else_contents.
else_contents ::= CS_OPEN ELSE CS_CLOSE contents.

comma_exprs ::= expr COMMA comma_exprs.
comma_exprs ::= expr.

expr ::= expr PLUS expr.
expr ::= expr MINUS expr.
expr ::= expr MULT expr.
expr ::= expr DIV expr.
expr ::= expr MOD expr.
expr ::= expr AND expr.
expr ::= expr OR expr.
expr ::= expr EQ expr.
expr ::= expr NE expr.
expr ::= expr GT expr.
expr ::= expr GE expr.
expr ::= expr LT expr.
expr ::= expr LE expr.
expr ::= NOT expr.
expr ::= NUM.
expr ::= STRING.
expr ::= VAR.
expr ::= VARNUM.

opt_req ::= COLON.
opt_req ::= BANG.
