package com.gim;
import com.gim.listener;

public class client 
{
	 public native int  init(listener lstr);
	 public native int stop();
	 public native int  login(String srvip, int srvport, String cliver, int enc, String cid, String pwd);
	 public native int disconnect(String cid);
	 public native int sendPeerMessage(String cid, String sn, String peercid, String data);
	 static {
	        System.loadLibrary("clientsdk");
	    }
}