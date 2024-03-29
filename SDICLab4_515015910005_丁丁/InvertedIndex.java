package sjtu.sdic.mapreduce;

import org.apache.commons.io.filefilter.WildcardFileFilter;
import sjtu.sdic.mapreduce.common.KeyValue;
import sjtu.sdic.mapreduce.core.Master;
import sjtu.sdic.mapreduce.core.Worker;

import java.io.File;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Created by Cachhe on 2019/4/24.
 */
public class InvertedIndex {

    public static List<KeyValue> mapFunc(String file, String value) {
        List<KeyValue> result = new ArrayList<>();
        Pattern pattern = Pattern.compile("[a-zA-Z0-9]+");
        Matcher matcher = pattern.matcher(value);
        while(matcher.find())
        {
            result.add(new KeyValue(matcher.group(), file));
        }
        return result;
    }

    public static String reduceFunc(String key, String[] values) {
        List<String> files = new ArrayList<>();
        for(int i = 0; i < values.length; ++i)
        {
            if(!files.contains(values[i]))
            {
                files.add(values[i]);
            }
        }
        String number = Integer.toString(files.size());
        String file = "";
        for(String tmp : files)
        {
            file += tmp;
            file += ",";
        }
        file = file.substring(0, file.length() - 1);
        String result = number + " " + file;
        return result;
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
                mr = Master.sequential("iiseq", files, 3, InvertedIndex::mapFunc, InvertedIndex::reduceFunc);
            } else {
                mr = Master.distributed("wcdis", files, 3, args[1]);
            }
            mr.mWait();
        } else {
            Worker.runWorker(args[1], args[2], InvertedIndex::mapFunc, InvertedIndex::reduceFunc, 100, null);
        }
    }
}
