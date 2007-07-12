create table nt_trans_strings (
  string_id integer not null primary key auto_increment,
  string text,

  index(string(150))
);

create table nt_trans_locs (
  loc_id integer not null primary key auto_increment,
  string_id integer not null,
  version integer default 0,
  filename varchar(255),
  location varchar(255),

  index(string_id),
  index(filename)
) TYPE=INNODB;

create table nt_trans_maps (
  map_id integer not null primary key auto_increment,
  string_id integer not null,
  lang char(2),
  string text,

  index(string_id)
) TYPE=INNODB;
