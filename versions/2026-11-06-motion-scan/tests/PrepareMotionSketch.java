import java.nio.file.*;
import java.io.*;
import processing.mode.java.preproc.PdePreprocessor;
public class PrepareMotionSketch {
  public static void main(String[] args) throws Exception {
    try(Writer out=Files.newBufferedWriter(Path.of(args[1]))){
      PdePreprocessor.builderFor("Kit32MotionTwin").setIsTesting(true).build()
        .write(out,Files.readString(Path.of(args[0])));
    }
  }
}
