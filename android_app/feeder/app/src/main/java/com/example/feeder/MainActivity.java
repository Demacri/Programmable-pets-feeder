package com.example.feeder;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.time.Instant;
import java.time.ZoneId;
import java.util.Set;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {
    static final UUID mUUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    private TextView infoTextView;
    private BluetoothSocket btSocket = null;
    private TextView timeTextField;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        infoTextView = ((TextView)findViewById(R.id.infoTextView));
        timeTextField = findViewById(R.id.timeTextField);
    }

    @Override
    protected void onStart() {
        super.onStart();
    }
    @Override
    protected void onResume() {
        super.onResume();
        boolean connected = connectToHC05();
        String info = "";
        if(connected) {
            setTime();
            reloadInfo();
        }

        infoTextView.setText(info);
    }

    private boolean connectToHC05() {
        BluetoothAdapter btAdapter = BluetoothAdapter.getDefaultAdapter();
        BluetoothDevice hc05 = null;
        Set<BluetoothDevice> bds = btAdapter.getBondedDevices();
        Log.d("MainActivity",bds.toString());
        for(BluetoothDevice bd : bds) {
            if(bd.getName().equals("HC-05")) {
                hc05 = bd;
                break;
            }
        }
        if(hc05 == null) return false;
        int c = 0;
        do {
            try {
                btSocket = hc05.createRfcommSocketToServiceRecord(mUUID);
                btSocket.connect();
                if (btSocket.isConnected()) {
                    Log.d("MainActivity","connesso a HC-05");
                    CharSequence text = "Connesso.";
                    Toast toast = Toast.makeText(getApplicationContext(), text, Toast.LENGTH_LONG);
                    toast.show();
                } else {
                    Log.d("MainActivity","impossibile connettersi a HC-05");
                    CharSequence text = "Impossibile connettersi al dispositivo bluetooth. [1]";
                    Toast toast = Toast.makeText(getApplicationContext(), text, Toast.LENGTH_LONG);
                    toast.show();
                }

            } catch (IOException e) {
                e.printStackTrace();
                CharSequence text = "Impossibile connettersi al dispositivo bluetooth. [2]";
                Toast toast = Toast.makeText(getApplicationContext(), text, Toast.LENGTH_LONG);
                toast.show();
            }
            c++;
        } while(!btSocket.isConnected() && c < 5);
        return btSocket != null;
    }

    public void onReloadInfoButtonClick(View v) {
        reloadInfo();
    }
    private void setTime() {
        try {
            long time = System.currentTimeMillis()/1000;
            int s = ZoneId.systemDefault().getRules().getOffset(Instant.ofEpochMilli( 1_484_063_246L )).getTotalSeconds();
            byte[] msg = ("T " + (time+s+3600) + ";").getBytes(); //wrong 1h --> 3600sec
            OutputStream outputStream = btSocket.getOutputStream();
            outputStream.write(msg);
            Log.d("MainActivity","set time:" + time);
        } catch (IOException e) {
            e.printStackTrace();
        }

    }
    private void reloadInfo() {

        try {
            OutputStream outputStream = btSocket.getOutputStream();
            outputStream.write("I;".getBytes());

            //inputStream.skip(inputStream.available());
            Thread one = new Thread() {
                public void run() {
                    int i = 0;
                    try {
                        InputStream inputStream = btSocket.getInputStream();
                        while (i < 3) {

                            Log.d("MainActivity","Input Stream Available?");
                            if(inputStream.available() > 0) {
                                Log.d("MainActivity","Input Stream Disponibile");
                                byte[] buffer = new byte[inputStream.available()];
                                inputStream.read(buffer);
                                final String info = new String(buffer);
                                findViewById(R.id.infoTextView).post(new Runnable() {
                                    public void run() {
                                        infoTextView.setText(info);
                                    }
                                });
                                Log.d("MainActivity","Info: " +info);
                                break;
                            }
                            Thread.sleep(1500);

                            Log.d("MainActivity","Input Stream vuoto. Ritenta " + (i+1) + "/3");
                            i++;
                        }
                    } catch (InterruptedException v) {
                        System.out.println(v);
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            };

            one.start();

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void onSaveBtnClick(View v) {
        String val = timeTextField.getText().toString();
        if(!val.matches("\\d{2}:\\d{2}")) {
            Toast.makeText(this,"Formato orario errato. [hh:mm]",Toast.LENGTH_LONG).show();
            return;
        }
        try {
            byte[] msg = ("S " + val + ";").getBytes();
            OutputStream outputStream = btSocket.getOutputStream();
            outputStream.write(msg);
            Log.d("MainActivity","set open time:" + val);
            reloadInfo();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        try {
            btSocket.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    @Override
    protected void onStop() {
        super.onStop();
    }
}
