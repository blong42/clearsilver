#!/neo/opt/bin/python

import sys, string, os, getopt, pwd, signal, time, re
import fcntl

import tstart

import db_trans
from log import *
import neo_cgi, neo_util
import odb

eTransError = "eTransError"

DONE = 0
DEBUG = 0

TIER2_DIV = 11
TIER1_DIV = 11 * TIER2_DIV

if not DEBUG: LOGGING_STATUS[DEV_UPDATE] = 0

def handleSignal(*arg):
  global DONE
  DONE = 1

def usage():
  print "usage info!!"

def exceptionString():
  import StringIO, traceback

  ## get the traceback message  
  sfp = StringIO.StringIO()
  traceback.print_exc(file=sfp)
  exception = sfp.getvalue()
  sfp.close()

  return exception

class TransLoc:
    def __init__ (self, string_id, filename, location):
        self.string_id = string_id
        self.filename = filename
        self.location = location

class Translator:
    _HTML_TAG_RE = None
    _HTML_TAG_REGEX = '<[^!][^>]*?>'
    _HTML_CMT_RE = None
    _HTML_CMT_REGEX = '<!--.*?-->'
    _CS_TAG_RE = None
    _CS_TAG_REGEX = '<\\?.+?\\?>'

    def __init__ (self):
        self.tdb = db_trans.trans_connect()

        # configuration data ......
        #  - we should stop hardcoding this... - jeske
        
        self.root = "testroot"
        self.languages = ['es', 'en'] 

        self.ignore_paths = ['tmpl/m']  # common place for mockups
        self.ignore_files = ['blah_ignore.cs'] # ignore clearsilver file

        # ignore clearsilver javascript files
        self.ignore_patterns = ['tmpl/[^ ]*_js.cs'] 

        # ............................


        if self.root is None:
            raise "Unable to determine installation root"


        if Translator._HTML_TAG_RE is None:
            Translator._HTML_TAG_RE = re.compile(Translator._HTML_TAG_REGEX, re.MULTILINE | re.DOTALL)
        if Translator._HTML_CMT_RE is None:
            Translator._HTML_CMT_RE = re.compile(Translator._HTML_CMT_REGEX, re.MULTILINE | re.DOTALL)
        if Translator._CS_TAG_RE is None:
            Translator._CS_TAG_RE = re.compile(Translator._CS_TAG_REGEX, re.MULTILINE | re.DOTALL)

        self._html_state = 0

       
    def parseHTMLTag(self, data):
        # this is only called if we see a full tag in one parse...
        i = 0
        if len(data) == 0: return []
        if data[0] in '/?': return []
        while i < len(data) and data[i] not in ' \n\r\t>': i = i + 1
        if i == len(data): return []
        tag = data[:i].lower()
        #print "Searching tag: %s" % data
        #print "Found tag: %s" % tag
        results = []
        attrfind = re.compile(
            r'\s*([a-zA-Z_][-.a-zA-Z_0-9]*)(\s*=\s*'
            r'(\'[^\']*\'|"[^"]*"|[^ \t\n<>]*))?')
        k = i
        attrs = {}
        attrs_beg = {}
        while k < len(data):
            match = attrfind.match(data, k)
            if not match: break
            attrname, rest, attrvalue = match.group(1, 2, 3)
            if not rest:
               attrvalue = attrname
            elif attrvalue[:1] == '\'' == attrvalue[-1:] or \
                 attrvalue[:1] == '"' == attrvalue[-1:]:
               attrvalue = attrvalue[1:-1]
            attrname = attrname.lower()
            if attrs.has_key(attrname):
                log("Can't handle duplicate attrs: %s" % attrname)
            attrs[attrname] = attrvalue
            attrs_beg[attrname] = match.start(3)
            k = match.end(0)

        find_l = []
        if tag == "input":
            if attrs.get('type', "").lower() in ["submit", "button"]:
                find_l.append((attrs.get('value', ''), attrs_beg.get('value', 0)))

        for s,k in find_l:
            if s:
                x = data[k:].find(s)
                if x != -1: results.append((s, x+k, 1))

        return results

    def parseHTML(self, data, reset=1):
        if reset: self._html_state = 0
        if DEBUG: print "- %d ---------\n%s\n- E ---------" % (self._html_state, data)

        results = []
        i = 0
        n = len(data)
        # if we had state from the last parse... find it
        if self._html_state:
            if self._html_state == 2:
                x = string.find(data[i:], '-->')
                l = 3
            else:
                x = string.find(data[i:], '>')
                l = 1
            if x == -1: return results
            i = i + x + l
            self._html_state = 0
        while i < n:
            if DEBUG: print "MATCHING>%s<MATCHING" % data[i:]
            cmt_b = string.find(data[i:], '<!--')
            cmt_e = string.find(data[i:], '-->')
            tag_b = string.find(data[i:], '<')
            tag_e = string.find(data[i:], '>')
            if DEBUG: print "B> %d %d %d %d <B" % (cmt_b, cmt_e, tag_b, tag_e)
            if cmt_b != -1 and cmt_b <= tag_b:
                x = i
                y = i+cmt_b-1
                while x < y and data[x] in string.whitespace: x+=1
                while y > x and data[y] in string.whitespace: y-=1
                results.append((data[x:y+1], x, 1))
                if cmt_e == -1: # partial comment:
                    self._html_state = 2
                    break
                i = i + cmt_e + 3
            elif tag_b != -1:
                x = i
                y = i+tag_b-1
                while x < y and data[x] in string.whitespace: x+=1
                while y > x and data[y] in string.whitespace: y-=1
                results.append((data[x:y+1], x, 1))
                if tag_e == -1: # partial tag
                    self._html_state = 1
                    break
                h_results = self.parseHTMLTag(data[i+tag_b+1:i+tag_e])
                h_results = map(lambda x: (x[0], x[1] + i+tag_b+1, x[2]), h_results)
                results = results + h_results
                i = i + tag_e + 1
            else:
                x = i
                y = n-1 
                while x < y and data[x] in string.whitespace: x+=1
                while y > x and data[y] in string.whitespace: y-=1
                results.append((data[x:y+1], x, 1))
                break
        return results

    def parseCS(self, data):
        results = []
        i = 0
        n = len(data)
        while i < n:
            m = Translator._CS_TAG_RE.search(data, i)
            if not m:
                # search for a partial...
                x = string.find(data[i:], '<?')
                if x == -1:
                    results.append((data[i:], i))
                else:
                    results.append((data[i:x], i))
                break
            (b, e) = m.span()
            if i != b: results.append((data[i:b], i))
            i = e 
        t_results = []
        self._html_in = 0
        for (s, ofs) in results:
            r = self.parseHTML(s, reset=0)
            r = map(lambda x: (x[0], x[1] + ofs, x[2]), r)
            t_results = t_results + r
        return t_results

    def descendHDF(self, obj, prefix):
        results = []
        while obj is not None:
            if obj.value():
                attrs = obj.attrs()
                attrs = map(lambda x: x[0], attrs)
                if "Lang" in attrs:
                    if prefix:
                        results.append((obj.value(), "%s.%s" % (prefix, obj.name()), 0))
                    else:
                        results.append((obj.value(), "%s" % (obj.name()), 0))
            if obj.child():
                if prefix:
                    results = results + self.descendHDF(obj.child(), "%s.%s" % (prefix, obj.name()))
                else:
                    results = results + self.descendHDF(obj.child(), (obj.name()))
            obj = obj.next()
        return results

    def parseHDF(self, data):
        # Ok, we handle HDF files specially.. the theory is, we only
        # extract entire HDF elements which have the attribute Lang
        hdf = neo_util.HDF()
        hdf.readString(data, 1)
        return self.descendHDF(hdf, "")

    def handleFile(self, file):
        if file in self.ignore_files: return []
        for a_re in self.ignore_patterns:
            if re.match(a_re,file): 
                return []
        fpath = self.root + '/' + file
        x = string.rfind(file, '.')
        if x == -1: return []
        data = open(fpath, 'r').read()
        ext = file[x:]
        strings = []
        if ext in ['.cst', '.cs']:
            strings = self.parseCS(data)
        elif ext in ['.html', '.htm']:
            strings = self.parseHTML(data)
        elif ext in ['.hdf']:
            strings = self.parseHDF(data)
        if len(strings):
            print "Found %d strings in %s" % (len(strings), file)
            return strings
        return []

    def walkDirectory(self, path):
        if path in self.ignore_paths: return []
        fpath = self.root + '/' + path
        files = os.listdir(fpath)
        dirs = []
        results = []
        for file in files:
            if file[0] == '.': continue
            fname = fpath + '/' + file
            if os.path.isdir(fname):
                dirs.append(file)
            else:
                strings = self.handleFile(path + '/' + file)
                if len(strings):
                    results.append((path + '/' + file, strings))
        for dir in dirs:
            if dir not in ["release"]:
                results = results + self.walkDirectory(path + '/' + dir)
        return results

    def cleanHtmlString(self, s):
        s = re.sub("\s+", " ", s)
        return string.strip(s)

    def containsWords(self, s, ishtml):
        if ishtml:
            s = string.replace(s, '&nbsp;', ' ')
            s = string.replace(s, '&quot;', '"')
            s = string.replace(s, '&copy;', '')
            s = string.replace(s, '&lt;', '<')
            s = string.replace(s, '&gt;', '>')
            s = string.replace(s, '&amp;', '&')
        for x in range (len (s)):
          n = ord(s[x])
          if (n>47 and n<58) or (n>64 and n<91) or (n>96 and n<123): return 1
        return 0
        
    def findString(self, s):
        rows = self.tdb.strings.fetchRows( ('string', s) )
        if len(rows) == 0:
            row = self.tdb.strings.newRow()
            row.string = s
            row.save()
            return row.string_id
        elif len(rows) > 1:
            raise eTransError, "String %s exists multiple times!" % s
        else:
            return rows[0].string_id

    def loadStrings(self, one_file=None, verbose=0):
        if one_file is not None:
            strings = self.handleFile(one_file)
            results = [(one_file, strings)]
        else:
            results = self.walkDirectory('tmpl')
        uniq = {}
        cnt = 0
        seen_hdf = {}
        for fname, strings in results:
            for (s, ofs, ishtml) in strings:
                if s and string.strip(s):
                    l = len(s)
                    if ishtml:
                        s = self.cleanHtmlString(s)
                    if self.containsWords(s, ishtml):
                        if type(ofs) == type(""): # HDF
                            if seen_hdf.has_key(ofs):
                                if seen_hdf[ofs][0] != s:
                                    log("Duplicate HDF Name %s:\n\t file %s = %s\n\t file %s = %s" % (ofs, seen_hdf[ofs][1], seen_hdf[ofs][0], fname, s))
                            else:
                                seen_hdf[ofs] = (s, fname)
                        try:
                            uniq[s].append((fname, ofs, l))
                        except KeyError:
                            uniq[s] = [(fname, ofs, l)]
                        cnt = cnt + 1
        print "%d strings, %d unique" % (cnt, len(uniq.keys()))
        fp = open("map", 'w')
        for (s, locs) in uniq.items():
            locs = map(lambda x: "%s:%s:%d" % x, locs)
            fp.write('#: %s\n' % (string.join(locs, ',')))
            fp.write('msgid=%s\n\n' % repr(s))

        log("Loading strings/locations into database")
        locations = []
        for (s, locs) in uniq.items():
            s_id = self.findString(s)
            for (fname, ofs, l) in locs:
                if type(ofs) == type(""): # ie, its HDF
                    location = "hdf:%s" % ofs
                else:
                    location = "ofs:%d:%d" % (ofs, l)
                loc_r = TransLoc(s_id, fname, location)
                locations.append(loc_r)
        return locations

    def stringsHDF(self, prefix, locations, lang='en', exist=0, tiered=0):
        hdf = neo_util.HDF()
        if exist and lang == 'en': return hdf
        done = {}
        locations.sort()
        maps = self.tdb.maps.fetchRows( ('lang', lang) )
        maps_d = {}
        for map in maps:
            maps_d[int(map.string_id)] = map
        strings = self.tdb.strings.fetchRows()
        strings_d = {}
        for string in strings:
            strings_d[int(string.string_id)] = string
        count = 0
        for loc in locations:
            s_id = int(loc.string_id)
            if done.has_key(s_id): continue
            try:
                s_row = maps_d[s_id]
                if exist: continue
            except KeyError:
                try:
                    s_row = strings_d[s_id]
                except KeyError:
                    log("Missing string_id %d, skipping" % s_id)
                    continue
            count = count + 1
            if tiered:
                hdf.setValue("%s.%d.%d.%s" % (prefix, int(s_id) / TIER1_DIV, int(s_id) / TIER2_DIV, s_id), s_row.string)
            else:
                hdf.setValue("%s.%s" % (prefix, s_id), s_row.string)
            done[s_id] = 1
        if exist == 1: log("Missing %d strings for lang %s" % (count, lang))
        return hdf

    def dumpStrings(self, locations, lang=None):
        log("Dumping strings to HDF")
        if lang is None:
            langs = ['en']
            sql = "select lang from nt_trans_maps group by lang"
            cursor = self.tdb.defaultCursor()
            cursor.execute(sql)
            rows = cursor.fetchall()
            for row in rows:
                langs.append(row[0])
        else:
            langs = [lang]

        for a_lang in langs:
            hdf = self.stringsHDF('S', locations, a_lang)
            hdf.writeFile("strings_%s.hdf" % a_lang)

        for a_lang in langs:
            hdf = self.stringsHDF('S', locations, a_lang, exist=1)
            if hdf.child():
                hdf.writeFile("strings_missing_%s.hdf" % a_lang)

    def fetchString(self, s_id, lang):
        if lang == "hdf":
            return "<?cs var:Lang.Extracted.%d.%d.%s ?>" % (int(s_id) / TIER1_DIV, int(s_id) / TIER2_DIV, s_id)
        rows = self.tdb.maps.fetchRows( [('string_id', s_id), ('lang', lang)] )
        if len(rows) == 0:
            try:
                row = self.tdb.strings.fetchRow( ('string_id', s_id) )
            except odb.eNoMatchingRows:
                log("Unable to find string id %s" % s_id)
                raise eNoString
            if lang != 'en':
                log("Untranslated string for id %s" % s_id)
            return row.string
        else:
            return rows[0].string

    def dumpFiles(self, locations, lang):
        log("Dumping files for %s" % lang)
        files = {}
        for row in locations:
            try:
                files[row.filename].append(row)
            except KeyError:
                files[row.filename] = [row]

        hdf_map = []

        os.system("rm -rf %s/gen/tmpl" % (self.root))
        for file in files.keys():
            fname = "%s/gen/%s" % (self.root, file)
            try:
                os.makedirs(os.path.dirname(fname))
            except OSError, reason:
                if reason[0] != 17:
                    raise
            do_hdf = 0
            x = string.rfind(file, '.')
            if x != -1 and file[x:] == '.hdf':
                do_hdf = 1
            ofs = []
            for loc in files[file]:
                parts = string.split(loc.location, ':')
                if len(parts) == 3 and parts[0] == 'ofs' and do_hdf == 0: 
                    ofs.append((int(parts[1]), int(parts[2]), loc.string_id))
                elif len(parts) == 2 and parts[0] == 'hdf' and do_hdf == 1:
                    hdf_map.append((parts[1], loc.string_id))
                else:
                    log("Invalid location for loc_id %s" % loc.loc_id)
                    continue
            if not do_hdf:
                ofs.sort()
                data = open(self.root + '/' + file).read()
                # ok, now we split up the original data into sections
                x = 0
                n = len(data)
                out = []
                #sys.stderr.write("%s\n" % repr(ofs))
                while len(ofs):
                    if ofs[0][0] > x:
                        out.append(data[x:ofs[0][0]])
                        x = ofs[0][0]
                    elif ofs[0][0] == x:
                        out.append(self.fetchString(ofs[0][2], lang))
                        x = ofs[0][0] + ofs[0][1]
                        ofs = ofs[1:]
                    else:
                        log("How did we get here? %s x=%d ofs=%d sid=%d" % (file, x, ofs[0][0], ofs[0][2]))
                        log("Data[x:20]: %s" % data[x:20])
                        log("Data[ofs:20]: %s" % data[ofs[0][0]:20])
                        break
                if n > x:
                    out.append(data[x:])
                odata = string.join(out, '')
                open(fname, 'w').write(odata)

        if lang == "hdf":
            langs = self.languages
        else:
            langs = [lang]

        for d_lang in langs:
          # dumping the extracted strings
          hdf = self.stringsHDF('Lang.Extracted', locations, d_lang, tiered=1)
          fname = "%s/gen/tmpl/lang_%s.hdf" % (self.root, d_lang)
          hdf.writeFile(fname)
          data = open(fname).read()
          fp = open(fname, 'w')
          fp.write('## AUTOMATICALLY GENERATED -- DO NOT EDIT\n\n')
          fp.write(data)
          fp.write('\n#include "lang_map.hdf"\n')

          # dumping the hdf strings file
          if d_lang == "en":
            map_file = "%s/gen/tmpl/lang_map.hdf" % (self.root)
          else:
            map_file = "%s/gen/tmpl/%s/lang_map.hdf" % (self.root, d_lang)
          try:
              os.makedirs(os.path.dirname(map_file))
          except OSError, reason: 
              if reason[0] != 17: raise
          map_hdf = neo_util.HDF()
          for (name, s_id) in hdf_map:
              str = hdf.getValue('Lang.Extracted.%d.%d.%s' % (int(s_id) / TIER1_DIV, int(s_id) / TIER2_DIV, s_id), '')
              map_hdf.setValue(name, str)
          map_hdf.writeFile(map_file)

    def loadMap(self, file, prefix, lang):
        log("Loading map for language %s" % lang)
        hdf = neo_util.HDF()
        hdf.readFile(file)
        obj = hdf.getChild(prefix)
        updates = 0
        new_r = 0
        while obj is not None:
            s_id = obj.name()
            str = obj.value()

            try:
                map_r = self.tdb.maps.fetchRow( [('string_id', s_id), ('lang', lang)])
            except odb.eNoMatchingRows:
                map_r = self.tdb.maps.newRow()
                map_r.string_id = s_id
                map_r.lang = lang
                new_r = new_r + 1

            if map_r.string != str:
                updates = updates + 1
                map_r.string = str
                map_r.save()

            obj = obj.next()
        log("New maps: %d  Updates: %d" % (new_r, updates - new_r))
        

def main(argv):
  alist, args = getopt.getopt(argv[1:], "f:v:", ["help", "load=", "lang="])

  one_file = None
  verbose = 0
  load_file = None
  lang = 'en'
  for (field, val) in alist:
    if field == "--help":
      usage(argv[0])
      return -1
    if field == "-f":
      one_file = val
    if field == "-v":
      verbose = int(val)
    if field == "--load":
        load_file = val
    if field == "--lang":
        lang = val
        

  global DONE

  #signal.signal(signal.SIGTERM, handleSignal)
  #signal.signal(signal.SIGINT, handleSignal)

  log("trans: start")

  start_time = time.time()

  try:
    t = Translator()
    if load_file:
        t.loadMap(load_file, 'S', lang)
    else:
        locations = t.loadStrings(one_file, verbose=verbose)
        t.dumpStrings(locations)
        t.dumpFiles(locations, 'hdf')
  except KeyboardInterrupt:
    pass
  except:
    import handle_error
    handle_error.handleException("Translation Error")

if __name__ == "__main__":
  main(sys.argv)
