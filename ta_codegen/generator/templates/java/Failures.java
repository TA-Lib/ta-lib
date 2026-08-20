/* Default-package twins of the shipped io.github.talib failure types (the
 * hand-written library scaffolding is the canonical copy — keep the two in
 * sync). The server calls the public wrapper on every correctness request
 * (#236 step 4), so the spliced fragment text has to compile, and it has to
 * compile against the SAME types the library ships, or the identity that splice exists to preserve would be an
 * identity of text only. */
interface TaLibFailure {
   RetCode retCode();
}

class TaLibArgumentException extends IllegalArgumentException implements TaLibFailure {
   private static final long serialVersionUID = 1L;
   private final RetCode retCode;

   TaLibArgumentException(String message, RetCode retCode) {
      super(message);
      this.retCode = retCode;
   }

   @Override
   public RetCode retCode() {
      return retCode;
   }
}

class TaLibIndexException extends IndexOutOfBoundsException implements TaLibFailure {
   private static final long serialVersionUID = 1L;
   private final RetCode retCode;

   TaLibIndexException(String message, RetCode retCode) {
      super(message);
      this.retCode = retCode;
   }

   @Override
   public RetCode retCode() {
      return retCode;
   }
}

class TaLibStateException extends IllegalStateException implements TaLibFailure {
   private static final long serialVersionUID = 1L;
   private final RetCode retCode;

   TaLibStateException(String message, RetCode retCode) {
      super(message);
      this.retCode = retCode;
   }

   @Override
   public RetCode retCode() {
      return retCode;
   }
}

class TaLibNullArgumentException extends NullPointerException implements TaLibFailure {
   private static final long serialVersionUID = 1L;
   private final RetCode retCode;

   TaLibNullArgumentException(String message, RetCode retCode) {
      super(message);
      this.retCode = retCode;
   }

   @Override
   public RetCode retCode() {
      return retCode;
   }
}
