#!/usr/bin/env python3
"""
csv_to_cfonb120_exact.py

Convert a semicolon-separated CSV (your sample format) into a CFONB120 file
producing records matching the 01/04/05/07 layout used by akretion/python-cfonb.

Usage:
  python csv_to_cfonb120_exact.py input.csv output.cfonb \
    --bank-code 15589 --desk 00000 --account 98765432100 --nb-dec 2

Notes:
- Make sure python-cfonb (cfonb package) is importable in your environment.
- Adapt command-line parameters to match your bank/account information.
"""
import csv
import argparse
import html
from datetime import datetime
from cfonb.parser.common import write_amount

# Field layout sizes are taken directly from the parser regex in common.py:
# ParserContent01, ParserContent04, ParserContent05, ParserContent07
def pad_left(s, length, fill=' '):
    s = '' if s is None else str(s)
    return s.rjust(length, fill)[:length]

def pad_right(s, length, fill=' '):
    s = '' if s is None else str(s)
    return s.ljust(length, fill)[:length]

def make_header(params, opening_balance=None, prev_date=None):
    # ParserContent01 ordering & sizes:
    # 2(record_code) 5(bank_code) 4(_1) 5(desk_code) 3(currency) 1(nb_of_dec)
    # 1(_2) 11(account_nb) 2(_3) 6(prev_date) 50(_4) 14(prev_amount) 16(_5)
    rec = []
    rec.append(pad_right('01', 2))
    rec.append(pad_right(params['bank_code'], 5))
    rec.append(pad_right('', 4))
    rec.append(pad_right(params['desk'], 5))
    rec.append(pad_right(params.get('currency', 'EUR'), 3))
    rec.append(pad_right(str(params['nb_dec']), 1))
    rec.append(pad_right('', 1))
    rec.append(pad_right(params['account'], 11))
    rec.append(pad_right('', 2))
    # prev_date: ddmmyy or blanks
    if prev_date:
        rec.append(prev_date.strftime('%d%m%y'))
    else:
        rec.append(pad_right('', 6))
    rec.append(pad_right('', 50))
    if opening_balance is not None:
        rec.append(pad_right(write_amount(opening_balance, params['nb_dec']), 14))
    else:
        rec.append(pad_right('', 14))
    rec.append(pad_right('', 16))
    line = ''.join(rec)
    return line[:120]

def make_footer(params, total_amount=None, next_date=None):
    # ParserContent07 ordering & sizes:
    # 2(record_code) 5(bank_code) 4(_1) 5(desk_code) 3(currency) 1(nb_of_dec)
    # 1(_2) 11(account_nb) 2(_3) 6(next_date) 50(_4) 14(next_amount) 16(_5)
    rec = []
    rec.append(pad_right('07', 2))
    rec.append(pad_right(params['bank_code'], 5))
    rec.append(pad_right('', 4))
    rec.append(pad_right(params['desk'], 5))
    rec.append(pad_right(params.get('currency', 'EUR'), 3))
    rec.append(pad_right(str(params['nb_dec']), 1))
    rec.append(pad_right('', 1))
    rec.append(pad_right(params['account'], 11))
    rec.append(pad_right('', 2))
    if next_date:
        rec.append(next_date.strftime('%d%m%y'))
    else:
        rec.append(pad_right('', 6))
    rec.append(pad_right('', 50))
    if total_amount is not None:
        rec.append(pad_right(write_amount(total_amount, params['nb_dec']), 14))
    else:
        rec.append(pad_right('', 14))
    rec.append(pad_right('', 16))
    return ''.join(rec)[:120]

_internal_seq = 0
def next_internal_code():
    global _internal_seq
    _internal_seq += 1
    # internal_code is 4 chars (alphanumeric); produce zero-padded numeric
    return pad_left(str(_internal_seq)[-4:], 4, '0')

def build_04_line(params, tx):
    # ParserContent04:
    # 2 '04'
    # 5 bank_code
    # 4 internal_code
    # 5 desk_code
    # 3 currency
    # 1 nb_of_dec
    # 1 _1
    # 11 account_nb
    # 2 operation_code
    # 6 operation_date
    # 2 reject_code
    # 6 value_date
    # 31 label
    # 2 _2
    # 7 reference
    # 1 exempt_code
    # 1 _3
    # 14 amount
    # 16 _4
    rec = []
    rec.append(pad_right('04', 2))
    rec.append(pad_right(params['bank_code'], 5))
    rec.append(pad_right(tx.get('internal_code', next_internal_code()), 4))
    rec.append(pad_right(params['desk'], 5))
    rec.append(pad_right(params.get('currency', 'EUR'), 3))
    rec.append(pad_right(str(params['nb_dec']), 1))
    rec.append(pad_right('', 1))
    rec.append(pad_right(params['account'], 11))
    rec.append(pad_right(tx.get('operation_code', ''), 2))
    rec.append(tx['operation_date'].strftime('%d%m%y'))
    rec.append(pad_right('', 2))
    rec.append(tx['value_date'].strftime('%d%m%y'))
    rec.append(pad_right(tx.get('label', ''), 31))
    rec.append(pad_right('', 2))
    rec.append(pad_right(tx.get('reference', ''), 7))
    rec.append(pad_right('', 1))
    rec.append(pad_right('', 1))
    # amount: write_amount handles sign; ensure it's numeric (float)
    rec.append(pad_right(write_amount(tx['amount'], params['nb_dec']), 14))
    rec.append(pad_right('', 16))
    return ''.join(rec)[:120]

def build_05_line(params, internal_code, operation_date, qualifier, additional_info):
    # ParserContent05:
    # 2 '05'
    # 5 bank_code
    # 4 internal_code
    # 5 desk_code
    # 3 currency
    # 1 nb_of_dec
    # 1 _1
    # 11 account_nb
    # 2 operation_code
    # 6 operation_date
    # 5 _2
    # 3 qualifier
    # 70 additional_info
    # 2 _3
    rec = []
    rec.append(pad_right('05', 2))
    rec.append(pad_right(params['bank_code'], 5))
    rec.append(pad_right(internal_code, 4))
    rec.append(pad_right(params['desk'], 5))
    rec.append(pad_right(params.get('currency', 'EUR'), 3))
    rec.append(pad_right(str(params['nb_dec']), 1))
    rec.append(pad_right('', 1))
    rec.append(pad_right(params['account'], 11))
    rec.append(pad_right('', 2))
    rec.append(operation_date.strftime('%d%m%y'))
    rec.append(pad_right('', 5))
    rec.append(pad_right(qualifier, 3))
    rec.append(pad_right(additional_info, 70))
    rec.append(pad_right('', 2))
    return ''.join(rec)[:120]

def split_into_chunks(s, size):
    s = '' if s is None else s
    return [s[i:i+size] for i in range(0, len(s), size)]

def read_csv_transactions(path):
    """
    Expecting rows like:
    header line: "Compte de chèques";"Compte...";****7230;12/12/2025;;824,54
    transactions: date;type;subtype;label;...;amount
    We'll attempt to detect the header row by 'Compte' and extract opening balance if present in column 5 (index 5)
    """
    txs = []
    opening_balance = None
    with open(path, encoding='utf-8', newline='') as f:
        reader = csv.reader(f, delimiter=';')
        for r in reader:
            if not r:
                continue
            # header / account summary
            first = html.unescape(r[0]) if len(r) > 0 else ''
            if 'Compte' in first:
                # try opening balance in last column
                if len(r) >= 6 and r[5].strip():
                    amt = r[5].strip().replace(' ', '').replace('\u00A0', '').replace(',', '.')
                    try:
                        opening_balance = float(amt)
                    except Exception:
                        opening_balance = None
                continue
            # parse transaction
            # date formats: dd/mm/YYYY or dd/mm/YY
            date_str = r[0].strip()
            dt = None
            for fmt in ('%d/%m/%Y', '%d/%m/%y'):
                try:
                    dt = datetime.strptime(date_str, fmt)
                    break
                except Exception:
                    pass
            if dt is None:
                raise ValueError('Unrecognized date: %r' % date_str)
            # label: attempt column 3 like your sample
            label = html.unescape(r[3].strip()) if len(r) > 3 else ''
            # amount last column
            amount_raw = r[-1].strip().replace(' ', '').replace('\u00A0', '').replace(',', '.')
            amount = float(amount_raw)
            reference = r[2].strip() if len(r) > 2 else ''
            # build transaction record
            txs.append({
                'operation_date': dt,
                'value_date': dt,
                'label': label,
                'amount': amount,
                'reference': reference,
            })
    return opening_balance, txs

def convert(csv_in, cfonb_out, params):
    opening_balance, txs = read_csv_transactions(csv_in)

    lines = []
    # header prev_date: use first tx date or today if none
    first_date = txs[0]['operation_date'] if txs else None
    header = make_header(params, opening_balance=opening_balance, prev_date=first_date)
    lines.append(header)

    total = 0.0
    for tx in txs:
        internal_code = next_internal_code()
        line04 = build_04_line(params, {**tx, 'internal_code': internal_code})
        lines.append(line04)
        total += tx['amount']
        # create 05 LIB lines by splitting label into 70-char chunks
        label_chunks = split_into_chunks(tx.get('label', ''), 70)
        for idx, chunk in enumerate(label_chunks):
            # first LIB line has qualifier 'LIB', subsequent ones also 'LIB' (bank may expect incremental behavior)
            qualifier = 'LIB'
            lines.append(build_05_line(params, internal_code, tx['operation_date'], qualifier, chunk))
        # add REF line if reference is present
        if tx.get('reference'):
            ref_chunks = split_into_chunks(tx['reference'], 70)
            for chunk in ref_chunks:
                lines.append(build_05_line(params, internal_code, tx['operation_date'], 'REF', chunk))
        # As example shows, some banks also use RCN/NPY/AAA qualifiers if you have more structured pieces.
        # You can add more 05 lines here as needed.
    # footer next_date: use last tx date or None
    last_date = txs[-1]['operation_date'] if txs else None
    footer = make_footer(params, total_amount=total, next_date=last_date)
    lines.append(footer)

    # Write CRLF and latin-1 encoding (example file looks ASCII/latin1)
    with open(cfonb_out, 'w', encoding='latin-1', newline='') as f:
        for l in lines:
            if len(l) != 120:
                # safety: if any line is shorter, pad with spaces
                l = l.ljust(120)
            f.write(l + '\r\n')
    print("Wrote CFONB120 file:", cfonb_out)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('csv_in')
    ap.add_argument('cfonb_out')
    ap.add_argument('--bank-code', required=True)
    ap.add_argument('--desk', required=True)
    ap.add_argument('--account', required=True)
    ap.add_argument('--nb-dec', default=2, type=int)
    ap.add_argument('--currency', default='EUR')
    args = ap.parse_args()
    params = {
        'bank_code': args.bank_code,
        'desk': args.desk,
        'account': args.account,
        'nb_dec': args.nb_dec,
        'currency': args.currency,
    }
    convert(args.csv_in, args.cfonb_out, params)

if __name__ == '__main__':
    main()