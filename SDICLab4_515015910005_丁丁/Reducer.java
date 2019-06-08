package sjtu.sdic.mapreduce.core;

import com.alibaba.fastjson.JSON;
import sjtu.sdic.mapreduce.common.KeyValue;
import sjtu.sdic.mapreduce.common.Utils;

import java.io.*;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.HashMap;

/**
 * Created by Cachhe on 2019/4/19.
 */
public class Reducer {

    /**
     * 
     * 	doReduce manages one reduce task: it should read the intermediate
     * 	files for the task, sort the intermediate key/value pairs by key,
     * 	call the user-defined reduce function {@code reduceF} for each key,
     * 	and write reduceF's output to disk.
     * 	
     * 	You'll need to read one intermediate file from each map task;
     * 	{@code reduceName(jobName, m, reduceTask)} yields the file
     * 	name from map task m.
     *
     * 	Your {@code doMap()} encoded the key/value pairs in the intermediate
     * 	files, so you will need to decode them. If you used JSON, you can refer
     * 	to related docs to know how to decode.
     * 	
     *  In the original paper, sorting is optional but helpful. Here you are
     *  also required to do sorting. Lib is allowed.
     * 	
     * 	{@code reduceF()} is the application's reduce function. You should
     * 	call it once per distinct key, with a slice of all the values
     * 	for that key. {@code reduceF()} returns the reduced value for that
     * 	key.
     * 	
     * 	You should write the reduce output as JSON encoded KeyValue
     * 	objects to the file named outFile. We require you to use JSON
     * 	because that is what the merger than combines the output
     * 	from all the reduce tasks expects. There is nothing special about
     * 	JSON -- it is just the marshalling format we chose to use.
     * 	
     * 	Your code here (Part I).
     * 	
     * 	
     * @param jobName the name of the whole MapReduce job
     * @param reduceTask which reduce task this is
     * @param outFile write the output here
     * @param nMap the number of map tasks that were run ("M" in the paper)
     * @param reduceF user-defined reduce function
     */
    public static void doReduce(String jobName, int reduceTask, String outFile, int nMap, ReduceFunc reduceF) {
        Map<String, String[]> maps = new HashMap<>();
        for(int i = 0; i < nMap; ++i)
        {
            String subfile = Utils.reduceName(jobName, i, reduceTask);
            File file = new File(subfile);
            Long filelength = file.length();
            byte[] content = new byte[filelength.intValue()];
            String con = "";
            try
            {
                FileInputStream f = new FileInputStream(file);
                f.read(content);
                f.close();
                con = new String(content, "UTF-8");
            }
            catch(FileNotFoundException e)
            {
                System.out.println("can't find the file\n");
            }
            catch(IOException e)
            {
                System.out.println("io exception\n");
            }
            catch(Exception e)
            {
                System.out.println("unsupported encoding\n");
            }
            List<KeyValue> subpairs = JSON.parseArray(con, KeyValue.class);
            for(KeyValue pair : subpairs)
            {
                if(maps.containsKey(pair.key))
                {
                    String[] olds = maps.get(pair.key);
                    String[] news = new String[olds.length + 1];
                    for(int index = 0; index < olds.length; ++index)
                    {
                        news[index] = olds[index];
                    }
                    news[olds.length] = pair.value;
                    maps.put(pair.key, news);
                }
                else
                {
                    String[] news = new String[1];
                    news[0] = pair.value;
                    maps.put(pair.key, news);
                }
            }
        }
        File outfile = new File(outFile);
        if(!outfile.exists())
        {
            try
            {
                outfile.createNewFile();
            }
            catch (Exception e)
            {
                System.out.println("exception\n");
            }
        }
        Map<String, String> resultmap = new TreeMap<>();
        for(Map.Entry<String, String[]> pair : maps.entrySet())
        {
            String result = reduceF.reduce(pair.getKey(), pair.getValue());
            resultmap.put(pair.getKey(), result);
        }
        String sortstring = JSON.toJSONString(resultmap);
        try
        {
            FileWriter filewriter = new FileWriter(outFile);
            filewriter.write(sortstring);
            filewriter.close();
        }
        catch (IOException e)
        {
            System.out.println("io exception\n");
        }
    }
}
