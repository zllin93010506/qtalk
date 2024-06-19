package com.quanta.qtalk;

public class UnSupportException extends Exception {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;

	public UnSupportException(Throwable e) {
		super(e);
	}

	public UnSupportException(String detailMessage) {
		super(detailMessage);
	}
	
}
