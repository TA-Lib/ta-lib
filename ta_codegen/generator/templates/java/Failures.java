/* Default-package twins of the shipped io.github.talib failure types (the
 * hand-written library scaffolding is the canonical copy — keep the two in
 * sync). The server calls the public wrapper on every correctness request
 * (#236 step 4), so the spliced fragment text has to compile, and it has to
 * compile against the SAME types the library ships, or the identity that splice exists to preserve would be an
 * identity of text only. */
interface TALibFailure {
   RetCode retCode();
}

class TALibArgumentException extends IllegalArgumentException implements TALibFailure {
   private static final long serialVersionUID = 1L;
   private final RetCode retCode;

   TALibArgumentException(String message, RetCode retCode) {
      super(message);
      this.retCode = retCode;
   }

   @Override
   public RetCode retCode() {
      return retCode;
   }
}

class TALibIndexException extends IndexOutOfBoundsException implements TALibFailure {
   private static final long serialVersionUID = 1L;
   private final RetCode retCode;

   TALibIndexException(String message, RetCode retCode) {
      super(message);
      this.retCode = retCode;
   }

   @Override
   public RetCode retCode() {
      return retCode;
   }
}

class TALibStateException extends IllegalStateException implements TALibFailure {
   private static final long serialVersionUID = 1L;
   private final RetCode retCode;

   TALibStateException(String message, RetCode retCode) {
      super(message);
      this.retCode = retCode;
   }

   @Override
   public RetCode retCode() {
      return retCode;
   }
}
