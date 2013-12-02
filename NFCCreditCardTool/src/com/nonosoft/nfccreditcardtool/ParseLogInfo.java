package com.nonosoft.nfccreditcardtool;

import java.math.BigInteger;
import java.util.Arrays;

import android.util.Log;

public class ParseLogInfo {
	String res = "";
	
    public static String toHex(byte[] bytes) {
        BigInteger bi = new BigInteger(1, bytes);
        return String.format("%0" + (bytes.length << 1) + "X", bi);
    }
    
    public int byteValue(byte b) {
    	int v=(int) b;
    	if(v<0) v=v+256;
    	return(v);
    }
	
	public static String bytesToString(byte bt[], int start, int length) {
		int i;
		String s ="";
		for(i=start;i<Math.min(start+length, bt.length);i++) {
			s=s+(char) bt[i];
		}
		return(s);
	}
	
	ParseLogInfo(byte[] data) {
		this.res = new String();
		if(data.length==17) {
			// Date
			this.res+=String.format("%02x/%02x/20%02x", data[13],data[12],data[11]);
			// Transaction type
			this.res+=(data[14]==0 ? " Payment" : " Withdrawal");
			// Amount
			String siamount = String.format("%02x%02x%02x", data[2],data[3],data[4]);
			this.res+=" "+(new Integer(siamount).intValue())+","+String.format("%02x", data[5])+"€";
		}
	}
}
