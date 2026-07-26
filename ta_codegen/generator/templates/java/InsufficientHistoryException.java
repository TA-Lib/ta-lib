/* Default-package twin of the shipped
 * io.github.talib.InsufficientHistoryException (the hand-written library
 * scaffolding is the canonical copy — keep the two in sync). */
class InsufficientHistoryException extends IllegalArgumentException {
   private static final long serialVersionUID = 1L;

   public InsufficientHistoryException(String message) {
      super(message);
   }
}
