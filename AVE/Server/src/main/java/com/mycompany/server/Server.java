package com.mycompany.server;

import java.util.logging.Logger;
import java.util.logging.Level;

import java.time.LocalTime;
import java.time.LocalDate;
import java.time.LocalDateTime;
import static java.nio.charset.StandardCharsets.UTF_8;

import org.eclipse.paho.client.mqttv3.*;

import org.json.simple.JSONObject;
import org.json.simple.JSONArray;
import org.json.simple.parser.ParseException;
import org.json.simple.parser.JSONParser;

import java.sql.*; 

public class Server
{
    static QueueMsg msgRx = new QueueMsg();
    static QueueMsg msgTx = new QueueMsg();

    public static void main(String[] args)
    {    
        MQTT mqttT = new MQTT();
        Core coreT = new Core();
        mqttT.start();
        coreT.start();
    }
}

class QueueMsg
{
    private Element head;
    private Element tail;
    
    public QueueMsg()
    {
        head = tail = null;
    }
    
    public synchronized boolean isEmpty()
    {
        return (head == null);
    }
    
    public synchronized void enQueue(String t, String m)
    {
        Element node = new Element(t, m);
        
        if (head == null)
            head = node;
        else
            tail.next = node;
        
        tail = node;
    }
    
    public synchronized Message deQueue()
    {
        Message e = null;
        
        if (head != null)
        {
            e = head.elem;
            head = head.next;
            
            if (head == null)
                tail = null;
        }    
        return e;
    }
}

class Element
{
    public Message elem;
    public Element next = null;
    
    public Element(String t, String m)
    {
        elem = new Message();
        elem.tpc = t;
        elem.msg = m;
    }
}

class Message
{
    public String tpc;
    public String msg;
}

class MQTT extends Thread
{
    MqttClient con;
    MqttConnectOptions opt;
    boolean sub = false;
    boolean pub = false;
    Message mtx;

    //String broker = "tcp://broker.mqttdashboard.com:1883";
    String broker = "tcp://test.mosquitto.org:1883";
    String client = "ave_Server";
    String username = "";
    String password = "";
    
    @Override
    public void run()
    {
        try
        {
            con = new MqttClient(broker, client);
            opt = new MqttConnectOptions();
            opt.setUserName(username);
            opt.setPassword(password.toCharArray());

            con.setCallback(new MqttCallback()
            {
                @Override
                public void connectionLost(Throwable cause)
                {
                    sub = false;
                }
                
                @Override
                public void messageArrived(String topic, MqttMessage message) throws Exception
                {
                    System.out.println("Received\n\tTopic: " + topic + "\n\tMessage: " + message.toString());

                    Server.msgRx.enQueue(topic, message.toString());                
                }
                
                @Override
                public void deliveryComplete(IMqttDeliveryToken token)
                {
                    System.out.println("Transmitted\n\tTopic: " + mtx.tpc + "\n\tMessage: " + mtx.msg);

                    pub = false;
                }
            });
            
            do
            {
                if (sub)
                {
                    if (!pub && !Server.msgTx.isEmpty())
                    {
                        mtx = Server.msgTx.deQueue();

                        pub = true;
                        con.publish(mtx.tpc, mtx.msg.getBytes(UTF_8), 2, false);
                    }
                }
                else
                {
                    if (con.isConnected())
                    {
                        con.subscribe("ave_Server");                    
                        System.out.println("Connected and subcribe to: ave_Server");
                        sub = true;
                        
                        if (pub)
                            con.publish(mtx.tpc, mtx.msg.getBytes(UTF_8), 2, false);
                    }
                    else
                    {
                        con.connect();
                    }
                }
            } while (true);
        }
        catch(MqttException ex)
        {
            Logger.getLogger(Server.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
}      

class Core extends Thread
{
    @Override
    public void run()
    {
        do
        {
            if (!Server.msgRx.isEmpty())
            {
                JSONParser jsPrs = new JSONParser();

                try
                {
                    Message mrx = Server.msgRx.deQueue();

                    JSONObject jsObj0 = (JSONObject)jsPrs.parse(mrx.msg);
                    JSONObject jsObj1 = (JSONObject)jsObj0.get("msgData");

                    if (jsObj0.get("msgType").equals("Boot"))
                    {
                        MysqlCon dbc = new MysqlCon();

                        if (dbc.QueryRes("SELECT `clients`.`client`, `topic` FROM `controllers` JOIN `clients` ON `controllers`.`client` = `clients`.`client` WHERE `mac` = '" + jsObj1.get("client").toString() + "'"))
                        {
                            String tpc = jsObj1.get("topic").toString();
                            jsObj1 = dbc.getData();
                            jsObj0.replace("msgData", jsObj1);
                            
                            Server.msgTx.enQueue(tpc, jsObj0.toJSONString());                            
                        }
                        
                        dbc.CloseCon();
                    }
                    else if (jsObj0.get("msgType").equals("Device"))
                    {
                        MysqlCon dbc = new MysqlCon();

                        if (dbc.QueryRes("SELECT `topic`, `client`, `sensors` FROM `clients` JOIN `devices` ON `clients`.`device` = `devices`.`device` WHERE `client` = '" + jsObj1.get("client").toString() + "'"))
                        {
                            jsObj1 = dbc.getData();
                            String tpc = jsObj1.remove("topic").toString();
                            jsObj0.replace("msgData", jsObj1);
                            
                            Server.msgTx.enQueue(tpc, jsObj0.toJSONString());                            
                        }
                        
                        dbc.CloseCon();
                    }
                    else if (jsObj0.get("msgType").equals("Config"))
                    {
                        MysqlCon dbc = new MysqlCon();

                        if (dbc.QueryRes("SELECT `topic`, `client`, `name` FROM `clients` JOIN `devices` ON `clients`.`device` = `devices`.`device` WHERE `client` = '" + jsObj1.get("client").toString() + "'"))
                        {
                            String sns = jsObj1.get("number").toString();
                        
                            jsObj1 = dbc.getData();
                            String tpc = jsObj1.get("topic").toString();

                            if (dbc.QueryRes("SELECT * FROM `configs` WHERE `client` = '" + jsObj1.get("client").toString() + "' AND `number` = '" + sns + "'") ||
                                dbc.QueryRes("SELECT * FROM `configs` WHERE `client` = '" + jsObj1.get("name").toString() + "' AND `number` = '" + sns + "'"))
                            {
                                jsObj1 = dbc.getData();
                                jsObj1.remove("code");
                                jsObj1.remove("client");
                                jsObj1.remove("refresh");
                                jsObj0.replace("msgData", jsObj1);

                                Server.msgTx.enQueue(tpc, jsObj0.toJSONString());
                            }
                        }
                      
                        dbc.CloseCon();
                    }
                    else if (jsObj0.get("msgType").equals("Range"))
                    {
                        MysqlCon dbc = new MysqlCon();

                        if (dbc.QueryRes("SELECT `topic`, `client`, `name` FROM `clients` JOIN `devices` ON `clients`.`device` = `devices`.`device` WHERE `client` = '" + jsObj1.get("client").toString() + "'"))
                        {
                            String sns = jsObj1.get("number").toString();
                        
                            jsObj1 = dbc.getData();
                            String tpc = jsObj1.get("topic").toString();
                            
                            LocalDateTime d_t = LocalDateTime.now();
                            String dat = d_t.toLocalDate().toString();                            
                            
                            if (dbc.QueryRes("SELECT * FROM `ranges` WHERE `client` = '" + jsObj1.get("client").toString() + "' AND `number` = '" + sns + "' AND (`enddate` >= '" + dat + "' OR `enddate` is NULL)") ||
                                dbc.QueryRes("SELECT * FROM `ranges` WHERE `client` = '" + jsObj1.get("name").toString() + "' AND `number` = '" + sns + "'"))
                            {
                                jsObj1 = dbc.getData();
                                jsObj1.remove("code");
                                jsObj1.remove("client");
                                jsObj1.remove("refresh");
                                jsObj1.remove("startdate");
                                jsObj1.remove("enddate");
                                jsObj0.replace("msgData", jsObj1);

                                Server.msgTx.enQueue(tpc, jsObj0.toJSONString());
                            }
                        }
                      
                        dbc.CloseCon();
                    }
                    else if (jsObj0.get("msgType").equals("Time"))
                    {
                        MysqlCon dbc = new MysqlCon();

                        if (dbc.QueryRes("SELECT `topic` FROM `clients` WHERE `client` = '" + jsObj1.get("client").toString() + "'"))
                        {
                            String tpc = dbc.getData().get("topic").toString();

                            LocalTime tim = LocalTime.now();
                            
                            jsObj1.replace("hh", tim.getHour());
                            jsObj1.replace("mm", tim.getMinute());
                            jsObj1.replace("ss", tim.getSecond());
                            
                            Server.msgTx.enQueue(tpc, jsObj0.toJSONString());                            
                        }
                        
                        dbc.CloseCon();
                    }
                    else if (jsObj0.get("msgType").equals("Sensor"))
                    {
                        LocalDateTime d_t = LocalDateTime.now();
                        
                        if (jsObj1.get("hh").toString().equals("23") && d_t.getHour() == 0)
                            d_t.minusDays(1);

                        if (jsObj1.get("hh").toString().equals("0") && d_t.getHour() == 23)
                            d_t.plusDays(1);

                        String dat = d_t.toLocalDate().toString();
                        String tim = jsObj1.get("hh").toString() + ":" + jsObj1.get("mm").toString() + ":" + jsObj1.get("ss").toString();
                        
                        MysqlCon dbc = new MysqlCon();

                        dbc.QueryRow("INSERT INTO `measures` (`client`, `sensor`, `range`, `value`, `date`, `time`) VALUES ('" + jsObj1.get("client").toString() + "', '" + jsObj1.get("sensor").toString() + "', '" + jsObj1.get("range").toString() + "', " + jsObj1.get("value").toString() + ", '" + dat + "', '" + tim + "')");

                        dbc.CloseCon();
                    }
                }
                catch(ParseException pe)
                {
                    System.out.println("position: " + pe.getPosition());
                    System.out.println(pe);
                }
            }
        } while (true);
    } 
}

class MysqlCon
{
    String host = "jdbc:mysql://localhost:3306";
    String db = "avedb";
    String username = "root";
    String password = "";
    
    Connection con = null;
    Statement sta = null;
    ResultSet res = null;
    int row = 0;

    public MysqlCon()
    {
        try
        {
            con = DriverManager.getConnection(host + "/" + db + "?serverTimezone=UTC", username, password);
            sta = con.createStatement();
        }
        catch(SQLException se)
        {
            System.out.println(se);
        }
    }
    
    public boolean QueryRes(String qry)
    {
        boolean ret = false;
        
        System.out.println(qry);
        
        try
        {
            if (res != null)
                res.close();
            
            res = sta.executeQuery(qry);
            ret = res.next();
        }
        catch(SQLException se)
        {
            System.out.println(se);
        }
        
        return (ret);
    }
    
    public int QueryRow(String qry)
    {
        System.out.println(qry);
        
        try
        {
            row = 0;
            row = sta.executeUpdate(qry);
        }
        catch(SQLException se)
        {
            System.out.println(se);
        }
        
        return row;
    }
    
    public void CloseCon()
    {
        row = 0;
        
        try
        {
            if (res != null)
                res.close();
        }
        catch(SQLException se)
        {
            System.out.println(se);
        }
        
        try
        {
            if (sta != null)
                sta.close();
        }
        catch(SQLException se)
        {
            System.out.println(se);
        }

        try
        {
            if (con != null)
                con.close();
        }
        catch(SQLException se)
        {
            System.out.println(se);
        }
    }
    
    public JSONObject getData()
    {
        JSONObject jsObj = new JSONObject();
        
        try
        {
            for  (int c = 1; c <= res.getMetaData().getColumnCount(); c++)
                jsObj.put(res.getMetaData().getColumnName(c), res.getObject(c));
            
            res.next();
        }
        catch(SQLException se)
        {
            System.out.println(se);
        }

        return jsObj;
    }
}
