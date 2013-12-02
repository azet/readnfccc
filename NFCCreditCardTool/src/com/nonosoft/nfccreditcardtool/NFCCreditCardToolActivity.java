package com.nonosoft.nfccreditcardtool;

// /home/rli/installations/pssi/pssi-1.0/pssi

import java.io.IOException;
import java.math.BigInteger;

import android.app.Activity;
import android.os.Bundle;
import android.os.Parcelable;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.IntentFilter.MalformedMimeTypeException;
//import android.content.res.Resources;
import android.nfc.NfcAdapter;
import android.nfc.Tag;
import android.nfc.tech.IsoDep;
import android.util.Log;
import android.widget.TextView;

public class NFCCreditCardToolActivity extends Activity {
	private static final int DIALOG_NFC_OFF = 1;
	private NfcAdapter mAdapter;
	private PendingIntent pendingIntent;
	private String[][] mTechLists;
	private IntentFilter[] mFilters;
	
	TextView tv1;
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        tv1 = (TextView) findViewById(R.id.tv1);
        tv1.setText("Waiting for card...");
        
        mAdapter = NfcAdapter.getDefaultAdapter(this);
  	    pendingIntent = PendingIntent.getActivity(this, 0, new Intent(this, getClass()).addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP), 0);
		IntentFilter ndef = new IntentFilter(NfcAdapter.ACTION_TAG_DISCOVERED);
		try {
			ndef.addDataType("*/*");
		} catch (MalformedMimeTypeException e) {
			throw new RuntimeException("fail", e);
		}
		mFilters = new IntentFilter[] { ndef, };
		mTechLists = new String[][] { new String[] { IsoDep.class.getName() } };
    }
    
    @Override
 	public void onResume() {
         super.onResume();
         NfcAdapter mAdapter = NfcAdapter.getDefaultAdapter(this);
         if (mAdapter == null || !mAdapter.isEnabled()) {
             showDialog(DIALOG_NFC_OFF);
         }
         mAdapter.enableForegroundDispatch(this, pendingIntent, mFilters, mTechLists);
     }
    
    @Override
 	public void onPause() {
         super.onPause();
         mAdapter.disableForegroundDispatch(this);
     }
    
    public static String toHex(byte[] bytes) {
        BigInteger bi = new BigInteger(1, bytes);
        return String.format("%0" + (bytes.length << 1) + "X", bi);
    }
    
    @Override
    protected void onNewIntent(Intent intent){
    	int i; 
    	
    	this.setIntent(intent);
    	tv1.setText("New intent");
    	
        Parcelable nfcTag = intent.getParcelableExtra("android.nfc.extra.TAG");
        Tag t = (Tag) nfcTag;
        IsoDep myTag = IsoDep.get(t);
        if( !myTag.isConnected() ) {
			try {
				myTag.connect();
			} catch (IOException e) {
				e.printStackTrace();
				return;
			}
        }
        tv1.setText("Tag connected");
        byte selectCommand[] = {0x00,(byte) 0xa4,0x04,0x00,0x07,(byte) 0xa0,0x00,0x00,0x00,0x42,0x10,0x10};
        byte response[] = new byte[100];
        try {
			response = myTag.transceive(selectCommand);
			tv1.setText("ATS received");
		} catch (IOException e) {
			e.printStackTrace();
			return;
		}
        byte readRecord[] = {0x00,(byte) 0xB2,0x02,0x0C,0x00};
        try {
			response = myTag.transceive(readRecord);
			ParseGeneralInfo pgi = new ParseGeneralInfo(response);
			tv1.setText(pgi.cardholdername+"\n"+pgi.pan+"\n"+pgi.expirydate+"\n");
		} catch (IOException e) {
			e.printStackTrace();
			return;
		}
        byte readPayLog[] = {0x00,(byte) 0xB2,0x01,(byte) 0x8C,0x00};
        for(i=1;i<=20;i++) {
        	readPayLog[2] = (byte) i;
	        try {
				response = myTag.transceive(readPayLog);
				ParseLogInfo pli = new ParseLogInfo(response);
				tv1.setText(tv1.getText()+"\n"+pli.res);
			} catch (IOException e) {
				e.printStackTrace();
				return;
			}
        }
    }
    
}