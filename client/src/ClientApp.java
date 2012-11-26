import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Date;
import java.util.concurrent.atomic.AtomicInteger;

import javax.swing.JButton;
import javax.swing.JFrame;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;

public class ClientApp implements Runnable {
	public AtomicInteger numEvents = new AtomicInteger();
	public static int numClients = 0;
	public static boolean inDebugMode = true;
	
	public static void populateMainPane(Container pane) {
			
		pane.setLayout(new GridBagLayout());
		
		JLabel connTableLabel = new JLabel("All Connection Events:");
		final ConnectionTable conns = new ConnectionTable();
		JScrollPane connScroll = new JScrollPane(conns);
		
		Object [] s = new Object[5];
		
//		for (int i = 0; i < 50; i++) {
//			for (int j = 0; j < 5; j++) {
//				try {
//					Thread.sleep(10);
//				} catch (InterruptedException e) {
//					// TODO Auto-generated catch block
//					e.printStackTrace();
//				}
//				if (j==0)
//					s[j] = numEvents.incrementAndGet();
//				if (j == 1)
//					s[j] = new Date();
//				else if (j==3 || j==4)
//					s[j] = "102.168.100." + (i % 5);
//				else
//					s[j] = i + j + 1;
//			}
//			
//			conns.addRow(s);
//		}
//				
		//Horizontal Rule
		JLabel serverNameLabel = new JLabel("Server Host Name:");
		final JTextField serverName = new JTextField("127.0.0.1", 15);
		JLabel serverPortLabel = new JLabel("Server Port:");
		final JTextField serverPort= new JTextField("5050",5);
		JLabel threadPriorityLabel = new JLabel("Client Thread Priority:");
		threadPriorityLabel.setToolTipText("Integer ³ 1");
		final JTextField threadPriority = new JTextField("" + ++numClients, 2);
		threadPriority.setToolTipText("Integer ³ 1");
				
		JButton createConnButton = new JButton("Create Connection");
		
		pane.add(connTableLabel, new GBC(0, 0));
		pane.add(connScroll, new GBC(0,1)
								.setSpan(4,1)
								.setFill(GridBagConstraints.BOTH)
								.setWeight(.2, .2));
		pane.add(serverNameLabel, new GBC(0, 2)
									.setAnchor(GBC.WEST));
		pane.add(serverName, new GBC(1, 2)
									.setAnchor(GBC.WEST));
		pane.add(serverPortLabel, new GBC(0, 3)
									.setAnchor(GBC.WEST));
		pane.add(serverPort, new GBC(1, 3)
									.setAnchor(GBC.WEST));
		pane.add(threadPriorityLabel, new GBC(0, 4)
									.setAnchor(GBC.WEST));
		pane.add(threadPriority, new GBC(1, 4)
									.setAnchor(GBC.WEST));
		pane.add(createConnButton, new GBC(0, 5)
										.setSpan(4, 1)
										.setAnchor(GBC.CENTER));
		createConnButton.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				new ClientConnection(conns,
						serverName.getText(), 
						Integer.parseInt(serverPort.getText()),
						Integer.parseInt(threadPriority.getText())).execute();
				threadPriority.setText("" + ++numClients);
			}
		});
	}
	
	public static void populateConnectionPane(Container pane) {
		pane.setLayout(new GridBagLayout());
		
		JButton createConn = new JButton("Ok");
		JLabel serverNameLabel = new JLabel("Server Name:");
		JTextField serverName = new JTextField(30);
		JTextField serverPort;
		
		GridBagConstraints c = new GridBagConstraints();
		c.fill = GridBagConstraints.HORIZONTAL;
		c.gridx = 0;
		c.gridy = 0;
		pane.add(serverNameLabel, c);
		
		c.fill = GridBagConstraints.NONE;
		c.gridx = 1;
		c.insets = new Insets(0, 10, 1, 1);
		pane.add(serverName, c);
	}
	
	
	/**
	 * Create the GUI and show it. For thread safety, this method should be
	 * invoked from the event-dispatching thread.
	 */
	private static void createAndShowGUI() {
		// Create and set up the window.
		JFrame frame = new JFrame("Video Client");
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		populateMainPane(frame.getContentPane());

		// Display the window.
		frame.pack();
		frame.setMinimumSize(frame.getSize());
		frame.setVisible(true);
		
	}
	
	public void run() {
		createAndShowGUI();
	}

	public static void main(String[] args) {
		// Schedule a job for the event-dispatching thread:
		// creating and showing this application's GUI.
		javax.swing.SwingUtilities.invokeLater(new ClientApp());
	}
}
