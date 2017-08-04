package gov.pnnl.emsl.ecce.jms;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.InputStream;
import java.io.PrintStream;

public class Execer extends Thread {

    private JMSDispatcher dispatcher;
    private String appName;
    private Process proc;
    private boolean verbose;

    public Execer(JMSDispatcher dispatch, String app, boolean verboseFlag) {

        appName = app;
        dispatcher = dispatch;
        verbose = verboseFlag;
    }
    
    // Inner class to filter process output to standard out
    private class StreamPrinter extends Thread {

        private InputStream p_stream;
        private PrintStream p_printer;

        public StreamPrinter(InputStream stream, PrintStream printer) {

            p_stream = stream;
            p_printer = printer;
        }
        
        public void run() {

            try {
                // Print the output:
                BufferedReader reader =
                    new BufferedReader(new InputStreamReader(p_stream));
                String x;
                while ((x = reader.readLine()) != null) {
                    p_printer.println(x);
                }
                reader.close();

            } catch (Exception e) {
                System.out.println(
                         "Error funnelling output from exec'd process");
                System.out.println(e.getMessage());
                e.printStackTrace();
            }
        }
    }

    public void run() {
        
        try {
            if (verbose) {
                System.out.println("starting execer for " + appName);
            }
            proc = Runtime.getRuntime().exec("/bin/sh -c " + appName);

            // Create new threads to funnel output/error
            StreamPrinter outWriter = new StreamPrinter(proc.getInputStream(),
                                                        System.out);
            StreamPrinter errWriter = new StreamPrinter(proc.getErrorStream(),
                                                        System.err);
            outWriter.start();
            errWriter.start();

            // Wait for original process to exit, then quit
            proc.waitFor();
            dispatcher.quit();

        } catch (Exception e) {
            System.out.println(e.getMessage());
            e.printStackTrace();
        }
    }

}



