package com.example.myfirstapp;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Environment;
import android.support.v4.content.FileProvider;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.logging.Logger;

import static java.security.AccessController.getContext;

public class MainActivity extends AppCompatActivity {
    public static final String EXTRA_MESSAGE = "com.example.myfirstapp.MESSAGE";
    public static final String LOG_TAG = "YOUR-TAG-NAME";

    static {
        System.loadLibrary("native-lib");
        System.loadLibrary("vectorAdd-lib"); //native-lib
        System.loadLibrary("benchmark-lib");
    }

    private native String vectorAdd(int dataSize);
    private native String getInfo();
    private native String benchmark();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        TextView tv = (TextView) findViewById(R.id.textView2);
        tv.setText(getInfo());
    }

    /** Called when the user taps the Send button */
    public void runVectorAdd(View view) {
        // Do something in response to button
//        Intent intent = new Intent(this, DisplayMessageActivity.class);
//        EditText editText = (EditText) findViewById(R.id.editText);
//        String message = editText.getText().toString();
//        intent.putExtra(EXTRA_MESSAGE, message);
//        startActivity(intent);
        ScrollView mainScrollView = (ScrollView)findViewById(R.id.scrollView);
        mainScrollView.fullScroll(ScrollView.FOCUS_UP);

        EditText editText = (EditText) findViewById(R.id.dataSize);
        int dataSize = Integer.parseInt(editText.getText().toString());
        TextView tv = (TextView) findViewById(R.id.textView2);

        tv.setText(vectorAdd(dataSize));
    }
    public void runNative(View view) {
        ScrollView mainScrollView = (ScrollView)findViewById(R.id.scrollView);
        mainScrollView.fullScroll(ScrollView.FOCUS_UP);
        TextView tv = (TextView) findViewById(R.id.textView2);
        tv.setText(getInfo());
    }

    // Run several time to log data for graph
    public void runGraphData(View view){
        String fileName = "graph-data.csv";

        File myFile = new File(getExternalFilesDirs(null)[0].getAbsolutePath(), fileName);
        myFile.delete();
        for (int i = 1; i < 100000; i += 1000){
            vectorAdd(i);
        }
        sendEmail(fileName);
    }

    public void runBenchmark(View view){
        // Set up file
        String fileName = "busSpeedDownload.csv";
        String fileName2 = "busSpeedReadback.csv";
        String fileName3 = "matrixMul.csv";
        File myFile = new File(getExternalFilesDirs(null)[0].getAbsolutePath(), fileName);
        File myFile2 = new File(getExternalFilesDirs(null)[0].getAbsolutePath(), fileName2);
        File myFile3 = new File(getExternalFilesDirs(null)[0].getAbsolutePath(), fileName3);
//        myFile.delete();
//        myFile2.delete();
        myFile3.delete();

        ScrollView mainScrollView = (ScrollView)findViewById(R.id.scrollView);
        mainScrollView.fullScroll(ScrollView.FOCUS_UP);
        TextView tv = (TextView) findViewById(R.id.textView2);
        tv.setText(benchmark());

        //sendEmail(fileName);
//        sendEmail(fileName2);
        sendEmail(fileName3);
    }

    // A callback method to save data to a file for further process
    public void writeFile(String message, String fileName){
        if (isExternalStorageWritable()){
            File[] root = getExternalFilesDirs(null);
            File dir = new File(root[0].getAbsolutePath());
            dir.mkdirs();

            File file = new File(dir, fileName);
            Log.e(LOG_TAG, file.getAbsolutePath());
            try {
                FileWriter out = new FileWriter(file, true);
                out.write(message);
                out.close();
            } catch (IOException e) {
                Log.e(LOG_TAG, e.toString());
            }
        }
    }

    /* Checks if external storage is available for read and write */
    public boolean isExternalStorageWritable() {
        String state = Environment.getExternalStorageState();
        if (Environment.MEDIA_MOUNTED.equals(state)) {
            return true;
        }
        return false;
    }

    public void sendEmail(String fileName){
        File filelocation = new File(getExternalFilesDirs(null)[0].getAbsolutePath(), fileName);
        //Uri path = Uri.fromFile(filelocation);
        Uri contentUri = FileProvider.getUriForFile(getApplicationContext(), getApplicationContext().getPackageName() + ".com.example.myfirstapp.provider", filelocation);

        Intent emailIntent = new Intent(Intent.ACTION_SEND);
        // Allow the new apps to read the private file
        emailIntent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);

        // set the type to 'email'
        emailIntent.setType("message/rfc822");
        //emailIntent .setType("vnd.android.cursor.dir/email");
        String to[] = {"khoa102@gmail.com"};
        emailIntent .putExtra(Intent.EXTRA_EMAIL, to);
        // the attachment
        emailIntent .putExtra(Intent.EXTRA_STREAM, contentUri);

        // the mail subject
        emailIntent .putExtra(Intent.EXTRA_SUBJECT, fileName);
        //emailIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK); // this will make such that when user returns to your app, your app is displayed, instead of the email app.
        try {
            startActivity(Intent.createChooser(emailIntent, "Send mail..."));
        } catch (android.content.ActivityNotFoundException ex) {
            Toast.makeText(MainActivity.this, "There are no email clients installed.", Toast.LENGTH_SHORT).show();
        }
    }
}
