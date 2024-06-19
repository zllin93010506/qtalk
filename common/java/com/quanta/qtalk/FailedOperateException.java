package com.quanta.qtalk;

public class FailedOperateException extends Exception {
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    public FailedOperateException(Throwable e) {
        super(e);
    }
    private String mDetailMessage = null;
    public FailedOperateException(String detailMessage) {
        super(detailMessage);
        mDetailMessage = detailMessage;
    }
    @Override
    public String getMessage()
    {
        if (mDetailMessage!=null)
            return mDetailMessage;
        return super.getMessage();
    }
    @Override
    public String getLocalizedMessage ()
    {
        if (mDetailMessage!=null)
            return mDetailMessage;
        return super.getLocalizedMessage();
    }
}
