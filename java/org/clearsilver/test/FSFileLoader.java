package org.clearsilver.test;

import org.clearsilver.CSFileLoader;
import org.clearsilver.CSUtil;
import org.clearsilver.HDF;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.Reader;
import java.nio.CharBuffer;
import java.util.List;

/**
 * Simple fileloader that reads files from filesystem. Useful as a reference
 * implementation and for comparing against filesystem implementations.
 *
 * @author Sergio Marti (smarti@google.com)
 */
public class FSFileLoader implements CSFileLoader {

  private static final int BUF_SIZE = 128;

  public String load(HDF hdf, String filename) throws IOException {
    if (filename.charAt(0) == File.separatorChar) {
      // Absolute path.
      return readFile(new File(filename));
    }
    List<String> loadPaths = CSUtil.getLoadPaths(hdf);
    File f = CSUtil.locateFile(loadPaths, filename);
    if (f == null) {
      throw new FileNotFoundException("Couldn't find file " + filename +
          " in paths: " + loadPaths);
    } else {
      return readFile(f);
    }
  }

  private static String readFile(File file) throws IOException {
    Reader reader = new InputStreamReader(new FileInputStream(file), "UTF-8");
    try {
      reader = new BufferedReader(reader);
      CharBuffer buf = CharBuffer.allocate(BUF_SIZE);
      StringBuilder sb = new StringBuilder();
      while (true) {
        int charCount = reader.read(buf);
        if (charCount == -1) {
          break;
        }
        buf.flip();
        sb.append(buf, 0, charCount);
      }
      return sb.toString();
    } finally {
      reader.close();
    }
  }
}
