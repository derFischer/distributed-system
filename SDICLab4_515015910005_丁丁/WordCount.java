package sjtu.sdic.mapreduce;

import org.apache.commons.io.filefilter.WildcardFileFilter;
import sjtu.sdic.mapreduce.common.KeyValue;
import sjtu.sdic.mapreduce.core.Master;
import sjtu.sdic.mapreduce.core.Worker;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Created by Cachhe on 2019/4/21.
 */
public class WordCount {

    public static List<KeyValue> mapFunc(String file, String value) {
        List<KeyValue> result = new ArrayList<>();
        Pattern pattern = Pattern.compile("[a-zA-Z0-9]+");
        Matcher matcher = pattern.matcher(value);
        while(matcher.find())
        {
            result.add(new KeyValue(matcher.group(), "1"));
        }
        return result;
    }

    public static String reduceFunc(String key, String[] values) {
        return Integer.toString(values.length);
    }

    public static void main(String[] args) {
        if (args.length < 3) {
            System.out.println("error: see usage comments in file");
        } else if (args[0].equals("master")) {
            Master mr;

            String src = args[2];
            File file = new File(".");
            String[] files = file.list(new WildcardFileFilter(src));
            if (args[1].equals("sequential")) {
                mr = Master.sequential("wcseq", files, 3, WordCount::mapFunc, WordCount::reduceFunc);
            } else {
                mr = Master.distributed("wcdis", files, 3, args[1]);
            }
            mr.mWait();
        } else {
            Worker.runWorker(args[1], args[2], WordCount::mapFunc, WordCount::reduceFunc, 100, null);
        }
    }
}
