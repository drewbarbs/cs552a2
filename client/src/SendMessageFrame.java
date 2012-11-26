import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.IOException;
import java.io.OutputStream;
import java.net.Socket;

import javax.swing.*;

public class SendMessageFrame extends JFrame{
	private JTextArea inputField;
	private JScrollPane scroll;
	private JButton sendButton;
	private ClientConnection cliConn;
	
	public SendMessageFrame(ClientConnection cliConn) {
		super("Send Message");
		this.cliConn = cliConn;
		inputField = new JTextArea(20,80);
		sendButton = new JButton("Send");
		scroll = new JScrollPane(inputField);
		try{
			inputField.setText(java.net.InetAddress.getLocalHost().getHostName());
		} catch (Exception e) {}
		performLayout();
		setupSendButton();
		pack();
		setVisible(true);
		
	}
	
	private void setupSendButton() {
		final SendMessageFrame that = this;
		sendButton.addActionListener( new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent arg0) {
				cliConn.sendString(inputField.getText());
				that.dispose();
			}
		});
	}

	private void performLayout() {
		setLayout(new GridBagLayout());
		add(scroll, new GBC(0,0).setFill(GBC.BOTH).setInsets(10, 10, 10, 10));
		add(sendButton, new GBC(0,1).setAnchor(GBC.CENTER));
		
	}
	
}
