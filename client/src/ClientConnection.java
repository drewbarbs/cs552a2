import java.net.*;
import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;
import java.awt.Container;
import java.awt.FlowLayout;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.awt.event.WindowStateListener;
import java.awt.image.BufferedImage;
import java.io.*;
import java.lang.*;

import javax.imageio.ImageIO;
import javax.media.jai.*;
import javax.swing.*;

public class ClientConnection extends SwingWorker<Object, ToPublish> {
	public static AtomicInteger numEvents = new AtomicInteger();
	
	public static int FIELD_NUM_MSG_LEN;
	public static int FIELD_NUM_DATA_LEN;
	
	private ConnectionTable globalTable;
	private ConnectionTable localEventsTable;
	private JFrame connFrame;
	private String serverHostname;
	private int serverPort;
	private int threadPriority;
	private boolean headless;
	private Socket skt;

	private volatile boolean playingMovie;

	private volatile boolean cancelled;

	private JFrame movFrame;
	
	public ClientConnection(ConnectionTable globalTable,
			String serverHostname, 
			int serverPort, 
			int threadPriority) {
		this.globalTable = globalTable;
		this.serverHostname = serverHostname;
		this.serverPort = serverPort;
		this.threadPriority = threadPriority;
		this.playingMovie = false;
		this.cancelled = false;
		this.connFrame = new JFrame(String.format("Client %d", threadPriority));
		this.localEventsTable = new ConnectionTable();
		this.movFrame = new JFrame("Movie");
		
		if (ClientApp.inDebugMode) {
			FIELD_NUM_MSG_LEN = 4;
			FIELD_NUM_DATA_LEN = 5;
		} else {
			FIELD_NUM_MSG_LEN = 1;
			FIELD_NUM_DATA_LEN = 2;
		}

		populateConnectionPane();
		final ClientConnection that = this;
		this.connFrame.addWindowListener(new WindowAdapter() {
			@Override
			public void windowClosing(WindowEvent evt) {
				cancelled = true;
			}
		});

		connFrame.pack();
		connFrame.setMinimumSize(this.connFrame.getSize());
		connFrame.setVisible(true);
	}
	
	public void setPlayingMovie(boolean status) {
		playingMovie = status;
	}
	
	
	
	public void populateConnectionPane() {
		Container pane = connFrame.getContentPane();
		pane.setLayout(new GridBagLayout());
		JPanel buttonsPanel = new JPanel();
		buttonsPanel.setLayout(new GridBagLayout());
				
		JButton startMovieButton = new JButton("start_movie");
		JButton stopMovieButton = new JButton("stop_movie");
		JButton seekMovieButton = new JButton("seek_movie");
		JButton sendMessageButton = new JButton ("Send a message");
		buttonsPanel.add(startMovieButton, new GBC(0,0));
		buttonsPanel.add(stopMovieButton, new GBC(1,0));
		buttonsPanel.add(seekMovieButton, new GBC(2,0));
		buttonsPanel.add(sendMessageButton, new GBC(3,0));
		JScrollPane tableScroll = new JScrollPane(localEventsTable); 
		pane.add(tableScroll, new GBC(0, 0)								
				.setFill(GridBagConstraints.BOTH)
				.setWeight(.2, .2));
		final ClientConnection that = this;
		startMovieButton.addActionListener( new ActionListener() {

			@Override
			public void actionPerformed(ActionEvent arg0) {
				String msg = String.format("client:%d:start_movie:sw,1", 
											that.threadPriority);
				sendString(msg);
				try {
					publish(new ToPublish(new Date(), threadPriority, 
							java.net.InetAddress.getLocalHost().getHostName(),
							serverHostname, msg, null));
				} catch (UnknownHostException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				that.setPlayingMovie(true);

			}
			
		});
		stopMovieButton.addActionListener( new ActionListener() {

			@Override
			public void actionPerformed(ActionEvent arg0) {
				String msg = String.format("client:%d:stop_movie:", 
											that.threadPriority); 
				sendString(msg);
				try {
					publish(new ToPublish(new Date(), threadPriority, 
							java.net.InetAddress.getLocalHost().getHostName(),
							serverHostname, msg, null));
				} catch (UnknownHostException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				that.setPlayingMovie(true);

			}
			
		});
		sendMessageButton.addActionListener( new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				new SendMessageFrame(that);				
			}
		});
		
		pane.add(buttonsPanel, new GBC(0,1)
								.setAnchor(GBC.CENTER));
	}

	public synchronized void sendString(String msg) {
		try {
			OutputStream out = skt.getOutputStream();
			out.write(msg.getBytes());
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	/* From 
	 * http://stackoverflow.com/questions/2877310/is-the-matlab-function-of-reshape-available-in-any-java-library
	 */
	
	public static byte[][] reshape(byte[][] A, int m, int n) {
        int origM = A.length;
        int origN = A[0].length;
        if(origM*origN != m*n){
            throw new IllegalArgumentException("New matrix must be of same area as matix A");
        }
        byte[][] B = new byte[m][n];
        byte[] A1D = new byte[A.length * A[0].length];

        int index = 0;
        for(int i = 0;i<A.length;i++){
            for(int j = 0;j<A[0].length;j++){
                A1D[index++] = A[i][j];
            }
        }

        index = 0;
        for(int i = 0;i<n;i++){
            for(int j = 0;j<m;j++){
                B[j][i] = A1D[index++];
            }

        }
        return B;
    }
	
	@Override
	protected void process(List<ToPublish> chunks) {
		for (ToPublish chunk : chunks) {
			String infoMsg = (ClientApp.inDebugMode && chunk.imgData != null ? 
					chunk.msg + new String(chunk.imgData) : chunk.msg);
			String myHostname = String.format("Client %d", threadPriority);
			try {
				myHostname = 
						java.net.InetAddress.getLocalHost().getHostName();
			} catch (UnknownHostException e) {
				e.printStackTrace();
			}
			localEventsTable.addRow(new Date(), threadPriority, 
					myHostname, serverHostname, infoMsg);
			globalTable.addRow(new Date(), threadPriority, 
 myHostname,
					serverHostname, infoMsg);
			if (chunk.imgData != null) {
//				byte[][] tmp = {chunk.imgData}; 
//				byte[][] img = reshape(tmp, 160, 120);  
//				InputStream in = new ByteArrayInputStream(chunk.imgData);
//				try {
//					BufferedImage frame = ImageIO.read(in);
//					movFrame.setLayout(new FlowLayout());
//					movFrame.getContentPane().add(
//							new JLabel(new ImageIcon(frame)));
//					movFrame.pack();
//					movFrame.setVisible(true);
//				} catch (IOException e) {
//					// TODO Auto-generated catch block
//					e.printStackTrace();
//				} finally {
//					try {in.close();} catch (Exception e) {}
//				}
			}
			
		}
	}
	
	@Override
	protected Object doInBackground() throws Exception {
		skt = new Socket(serverHostname, serverPort);
		byte[] buf = new byte[128 * 1024];
		InputStream is = skt.getInputStream();
		while (!cancelled) {
			while (playingMovie) {
				Thread.sleep(1000);
				int numRead = is.read(buf);
				byte[] readBytes = buf.clone(); 
				if (numRead > 0) {
					while (readBytes != null) { 
					String[] msgFields = (new String(readBytes)).split(":");
					while (msgFields.length < FIELD_NUM_DATA_LEN) {
						int numAddtlRead = is.read(readBytes, numRead, 300);
						numRead += numAddtlRead;
						msgFields = (new String(readBytes)).split(":");
					}
					int msgLen = Integer.parseInt(msgFields[FIELD_NUM_MSG_LEN-1]);
					int dataLen = Integer.parseInt(msgFields[FIELD_NUM_DATA_LEN-1]);
					int debugStrLen = 0;
					for (int i = 0; i < FIELD_NUM_MSG_LEN; i++) {
						debugStrLen += msgFields[i].length();
					}
					int endOfMsg = debugStrLen + (ClientApp.inDebugMode ? 1 : 0)
							+ msgFields[FIELD_NUM_MSG_LEN-1].length() + 1
							+ msgFields[FIELD_NUM_DATA_LEN-1].length() + 2;
					while (numRead < endOfMsg + dataLen) {
						int numAddtlRead = is.read(readBytes, numRead, (endOfMsg + dataLen - numRead));
						numRead += numAddtlRead;
					}
					final byte[] debugMsgBytes = Arrays.copyOfRange(readBytes, 0, endOfMsg);
					final byte[] dataBytes = Arrays.copyOfRange(readBytes, endOfMsg, endOfMsg+dataLen);
					if (endOfMsg + dataLen < numRead) {
						byte[] tmp = new byte[numRead-endOfMsg - dataLen];
						System.arraycopy(readBytes, endOfMsg + dataLen, tmp, 0, tmp.length);
						readBytes = tmp;
						numRead -= (endOfMsg+dataLen);
//						readBytes = Arrays.copyOfRange(readBytes, 
//								endOfMsg + dataLen, readBytes.length);
					} else {
						readBytes = null;
					}
					String debugMsg = new String(debugMsgBytes);
					publish(new ToPublish(new Date(), threadPriority, 
							java.net.InetAddress.getLocalHost().getHostName(),
							serverHostname, debugMsg, dataBytes));
					}
				}
			}
		}
		skt.close();
		return null;
	}
	
}

class ToPublish {
	int threadPriority;
	Date date;
	String src;
	String dest;
	byte[] imgData;
	String msg;
	
	public ToPublish(Date date, int threadPriority, String src, String dest, 
						String msg, byte[] imgData) {
		this.date = date;
		this.threadPriority = threadPriority;
		this.src = src;
		this.dest = dest;
		this.msg = msg;
		this.imgData = imgData;
	}
	
}
