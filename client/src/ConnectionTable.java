import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.text.SimpleDateFormat;
import java.util.Comparator;
import java.util.Date;
import java.util.concurrent.atomic.AtomicInteger;

import javax.swing.JButton;
import javax.swing.JFrame;

import javax.swing.*;
import javax.swing.table.AbstractTableModel;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.JTableHeader;
import javax.swing.table.TableModel;
import javax.swing.table.TableRowSorter;

public class ConnectionTable extends JTable {
	private final static String[] COLUMNS = { "Event Number", "Time",
			"Thread Priority", "Source", "Destination", "Info" };

	private DefaultTableModel tableModel;

	private AtomicInteger eventNumber = new AtomicInteger(0);

	public ConnectionTable() {
		/* Instantiate a new table whose cells cannot be edited */
		super(new DefaultTableModel() {
			public boolean isCellEditable(int i, int j) {
				return false;
			}
		});
		tableModel = (DefaultTableModel) getModel();
		init();
	}

	private void init() {
		setupColumns();
		setupSorter();
		setupRowListener();
		setPreferredScrollableViewportSize(new Dimension(900, 200));
		setFillsViewportHeight(true);
		setShowGrid(false);
	}

	/*
	 * Code for row listener adapted from
	 * http://stackoverflow.com/questions/4317510
	 * /sorting-a-table-breaks-the-tablerowlistener
	 */
	private void setupRowListener() {
		addMouseListener(new MouseAdapter() {
			@Override
			public void mouseClicked(MouseEvent evt) {
				if (evt.getClickCount() >= 2) {
					Point p = new Point(evt.getX(), evt.getY());
					int row = rowAtPoint(p);
					if (row < 0) // Happens if user double clicks on area of
						return; // table that doesn't have a row

					/*
					 * Check if there is already an open window for this event.
					 */
					Frame[] currFrames = Frame.getFrames();
					for (Frame frame : currFrames)
						if (frame.isVisible()
								&& frame.getTitle().equals(
										"Event " + getValueAt(row, 0))) {
							return;
						}
					/*
					 * Make a new frame to display info about this event.
					 */
					JFrame jf = new JFrame("Event " + getValueAt(row, 0));
					jf.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
					jf.pack();
					jf.setLocationRelativeTo(null); // Center frame on screen
					jf.setVisible(true);
				}
			}
		});
	}

	private void setupColumns() {
		for (String column : COLUMNS)
			tableModel.addColumn(column);
		getColumnModel().getColumn(1).setCellRenderer(new DateRenderer());
		setAutoResizeMode(JTable.AUTO_RESIZE_LAST_COLUMN);
		/*
		 * Need to set max widths for all other columns for
		 * AUTO_RESIZE_LAST_COLUMN to work as you would expect
		 */
		getColumnModel().getColumn(0).setPreferredWidth(90);
		getColumnModel().getColumn(1).setPreferredWidth(100);
		getColumnModel().getColumn(2).setPreferredWidth(100);
		getColumnModel().getColumn(3).setPreferredWidth(150);
		getColumnModel().getColumn(4).setPreferredWidth(150);
		getColumnModel().getColumn(0).setMaxWidth(100);
		getColumnModel().getColumn(1).setMaxWidth(100);
		getColumnModel().getColumn(2).setMaxWidth(150);
		/*
		 * Set the max widths of Src, Dest columns ridiculously large to account
		 * for really long host names
		 */
		getColumnModel().getColumn(3).setMaxWidth(1000);
		getColumnModel().getColumn(4).setMaxWidth(1000);
	}

	public void setupSorter() {
		TableRowSorter<TableModel> sorter = new TableRowSorter<TableModel>(
				tableModel);
		sorter.setComparator(0, new IntegerComparator()); // Sort Event Number
		sorter.setComparator(1, new DateComparator()); // Sort Date
		sorter.setComparator(2, new IntegerComparator()); // Sort Thread
															// Priority
		setRowSorter(sorter);
	}

	private void addRow(Object[] row) {
		tableModel.addRow(row);
	}

	public void addRow(Date time, int priority, String src, String dest,
			String info) {
		Object[] row = { eventNumber.incrementAndGet(), time, priority, src,
				dest, info };
		addRow(row);
	}

	/*
	 * Helper Classes
	 */

	// Class used for formatting the "Time" column
	class DateRenderer extends DefaultTableCellRenderer {
		SimpleDateFormat formatter = new SimpleDateFormat("hh:mm:ss.SSS");;

		public DateRenderer() {
			super();
		}

		public void setValue(Object value) {
			setText((value == null) ? "" : formatter.format(value));
		}
	}

	/* Define Comparators used to sort the columns of the table */

	class IntegerComparator implements Comparator<Integer> {
		public int compare(Integer int1, Integer int2) {
			return int1.compareTo(int2);
		}
	}

	class DateComparator implements Comparator<Date> {
		public int compare(Date d1, Date d2) {
			return d1.compareTo(d2);
		}
	}
}
