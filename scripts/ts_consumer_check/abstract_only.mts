import { type Core, type OutRange, type ParamHolder, type RetCodeName, TALibError } from 'ta-lib/js/abstract';

export function callHolder(holder: ParamHolder): OutRange {
  return holder.call();
}
export const holderCore = (holder: ParamHolder): Core => holder.core;
export const failureCode = (err: unknown): RetCodeName | undefined => (err instanceof TALibError ? err.code : undefined);
