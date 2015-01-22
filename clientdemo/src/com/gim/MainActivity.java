package com.gim;


import org.json.JSONObject;
import org.json.JSONException;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.gim.client;
import com.gim.listener;

public class MainActivity extends Activity {

	public int sn;
	public String cidA;
	public EditText editCidA, editToA, editDataA;
	public Button btnLoginA,btnLogoutA, btnSendA;
	public TextView tvMsgA;
	
	public String cidB;
	public EditText editCidB, editToB, editDataB;
	public Button btnLoginB,btnLogoutB, btnSendB;
	
	public client cli;
	
	public String srvip;
	public int srvport;
	
	public Handler handle;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		cli = new client();
		cli.init(new listener() {
			@Override
			public void handleMessage(String s)
			{
				 try {
					    Message m = new Message();
		                m.what = 123;
		                m.obj = s;
		                handle.sendMessage(m);
			        } 
				 catch (Exception e) {
			            e.printStackTrace();
			        }
			}
			
		});
		
		sn = 55550000;
		
		srvip = new String("114.215.85.30");
		srvport = 3000;
		///AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
		editCidA = (EditText)findViewById(R.id.editCidA);
		editToA = (EditText)findViewById(R.id.editToA);
		editDataA = (EditText)findViewById(R.id.editDataA);
		btnLoginA = (Button)findViewById(R.id.btnLoginA);
		btnLogoutA = (Button)findViewById(R.id.btnLogoutA);
		btnSendA = (Button)findViewById(R.id.btnSendA);
		tvMsgA = (TextView)findViewById(R.id.tvMsgsA);
		editCidA.setText("test0001");
		editToA.setText("test0002");
		editDataA.setText("haha");
		btnLoginA.setOnClickListener(new OnClickListener() 
		{	
			@Override
			public void onClick(View arg0)
			{
				// TODO Auto-generated method stub
				String strCid = editCidA.getText().toString();

				cidA = strCid;
				cli.login(srvip, srvport, "client-test-147", 0, strCid, "password");
			}
		});
		btnLogoutA.setOnClickListener(new OnClickListener() 
		{	
			@Override
			public void onClick(View arg0)
			{
				// TODO Auto-generated method stub
				cli.disconnect(cidA);
			}
		});
		btnSendA.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				String strData = editDataA.getText().toString();
				
				String strTo = editToA.getText().toString();
				cli.sendPeerMessage(cidA, "2001", strTo, strData);
			}
		});
		
		/////BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB
		editCidB = (EditText)findViewById(R.id.editCidB);
		editToB = (EditText)findViewById(R.id.editToB);
		editDataB = (EditText)findViewById(R.id.editDataB);
		btnLoginB = (Button)findViewById(R.id.btnLoginB);
		btnLogoutB = (Button)findViewById(R.id.btnLogoutB);
		btnSendB = (Button)findViewById(R.id.btnSendB);
		editCidB.setText("test0002");
		editToB.setText("test0001");
		editDataB.setText("hehe");
		
		btnLoginB.setOnClickListener(new OnClickListener() 
		{	
			@Override
			public void onClick(View arg0)
			{
				// TODO Auto-generated method stub
				String strCid = editCidB.getText().toString();

				cidB = strCid;
				cli.login(srvip, srvport, "client-test-147", 0, strCid, "password");
			}
		});
		btnLogoutB.setOnClickListener(new OnClickListener() 
		{	
			@Override
			public void onClick(View arg0)
			{
				// TODO Auto-generated method stub
				cli.disconnect(cidB);
			}
		});
		btnSendB.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				String strData = editDataB.getText().toString();
				
				String strTo = editToB.getText().toString();
				
				cli.sendPeerMessage(cidB, "2001", strTo, strData);
			}
		});
		
		
		//handle msg
		handle = new Handler() {
        public void handleMessage(Message msg) { /* 这里是处理信息的方法 */
            switch (msg.what) {
            case 123: /* 在这处理要TextView对象Show时间的事件 */
            	String his = tvMsgA.getText().toString();
            	if(tvMsgA.getTextSize() > 200)
            	{
            		his = "";
            	}
            		
            	tvMsgA.setText((String)msg.obj);
                break;
            }
            super.handleMessage(msg);
        }
    };
	
	}
	
}
