#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import csv
import re
from datetime import datetime
from decimal import Decimal, InvalidOperation

DATE_RE = re.compile(r"^\d{2}/\d{2}/\d{4}$")

CREDIT_MAP = {"0": "{", "1": "A", "2": "B", "3": "C", "4": "D", "5": "E", "6": "F", "7": "G", "8": "H", "9": "I"}
DEBIT_MAP = {"0": "}", "1": "J", "2": "K", "3": "L", "4": "M", "5": "N", "6": "O", "7": "P", "8": "Q", "9": "R"}

CFONB_WIDTH = 120
AMOUNT_WIDTH = 14

def parse_iban_fr(iban: str):
    iban = iban.replace(" ", "").upper()
    if not iban.startswith("FR") or len(iban) < 27:
        raise ValueError("Expected a French IBAN (FR...").")
    bban = iban[4:]  # FRkk + BBAN
    bank_code = bban[0:5]
    branch_code = bban[5:10]
    account_number = bban[10:21]
    account_key = bban[21:23]
    if not (bank_code.isdigit() and branch_code.isdigit() and account_number.isdigit() and account_key.isdigit()):
        raise ValueError("IBAN BBAN fields must be numeric.")
    return bank_code, branch_code, account_number, account_key

def parse_amount(raw: str):
    if raw is None:
        return None
    s = raw.strip()
    if not s:
        return None
    s = s.replace(" ", "").replace("\u00a0", "")
    s = s.replace(",", ".")
    try:
        return Decimal(s)
    except InvalidOperation as exc:
        raise ValueError(f"Invalid amount: {raw}") from exc

def cfonb_encode_amount(amount: Decimal, decimals: int = 2, width: int = AMOUNT_WIDTH):
    sign = 1 if amount >= 0 else -1
    amount = abs(amount)
    scaled = (amount * (10 ** decimals)).quantize(Decimal("1"))
    digits = f"{int(scaled):d}"
    if len(digits) > width:
        raise ValueError(f"Amount too large for CFONB width {width}: {digits}")
    digits = digits.rjust(width, "0")
    last_digit = digits[-1]
    encoded_last = CREDIT_MAP[last_digit] if sign >= 0 else DEBIT_MAP[last_digit]
    return digits[:-1] + encoded_last

def sanitize_label(label: str):
    label = " ".join(label.strip().split())
    label = label.upper()
    return label.encode("latin1", "replace").decode("latin1")

def build_line(rec_type, bank_code, branch_code, account_number, date_ddmmyy="", amount_str="", name="", ref="", comp_type="", comp_info=""):
    line = [" "] * CFONB_WIDTH

    def put(start, end, value, align="left"):
        width = end - start
        value = value[:width]
        if align == "right":
            value = value.rjust(width, " ")
        else:
            value = value.ljust(width, " ")
        line[start:end] = list(value)

    # Fixed numeric headers
    put(0, 2, rec_type)
    put(2, 7, bank_code)       # 5 digits
    put(11, 16, branch_code)   # 5 digits
    put(16, 19, "EUR")
    put(19, 20, "2")
    put(21, 32, account_number)  # 11 digits
    put(34, 40, date_ddmmyy)

    if rec_type == "05":
        put(45, 48, comp_type)
        put(48, 118, comp_info)
    else:
        put(48, 79, name)
        put(81, 88, ref)

    if amount_str:
        put(90, 104, amount_str, align="right")

    return "".join(line)

def parse_csv_transactions(csv_path):
    transactions = []
    opening_balance = None

    with open(csv_path, newline="", encoding="utf-8") as f:
        reader = csv.reader(f, delimiter=";")
        started = False
        for row in reader:
            if not row:
                continue
            row = row + [""] * (6 - len(row))

            if not started:
                if DATE_RE.match(row[0].strip()):
                    started = True
                else:
                    continue

            # detect opening balance row
            if not DATE_RE.match(row[0].strip()) and DATE_RE.match(row[3].strip()):
                opening_balance = parse_amount(row[5])
                continue

            if not DATE_RE.match(row[0].strip()):
                continue

            date_str = row[0].strip()
            amount = parse_amount(row[4])
            if amount is None:
                continue

            label_parts = [row[1], row[2], row[3]]
            label = " ".join([p for p in label_parts if p and p.strip()])

            transactions.append(
                {
                    "date": datetime.strptime(date_str, "%d/%m/%Y").date(),
                    "amount": amount,
                    "label": sanitize_label(label),
                }
            )

    return opening_balance, transactions

def main():
    parser = argparse.ArgumentParser(description="Convert CSV to CFONB120.")
    parser.add_argument("input_csv", help="Input CSV path")
    parser.add_argument("output_cfo", help="Output CFONB (.cfo) path")
    parser.add_argument("--iban", default="FR76 3000 4022 3100 0101 7355 454", help="Account IBAN (FR)")
    parser.add_argument(
        "--balance-mode",
        choices=["opening", "closing"],
        default="opening",
        help="If CSV has a balance row, treat it as opening or closing balance.",
    )
    args = parser.parse_args()

    bank_code, branch_code, account_number, _ = parse_iban_fr(args.iban)

    opening_balance, transactions = parse_csv_transactions(args.input_csv)
    if not transactions:
        raise SystemExit("No transactions found in CSV.")

    transactions.sort(key=lambda t: t["date"])
    start_date = transactions[0]["date"]
    end_date = transactions[-1]["date"]

    total = sum(t["amount"] for t in transactions)

    if opening_balance is None:
        opening_balance = Decimal("0.00")

    if args.balance_mode == "opening":
        start_balance = opening_balance
        end_balance = opening_balance + total
    else:
        end_balance = opening_balance
        start_balance = opening_balance - total

    lines = []

    # Opening (01)
    lines.append(
        build_line(
            "01",
            bank_code,
            branch_code,
            account_number,
            date_ddmmyy=start_date.strftime("%d%m%y"),
            amount_str=cfonb_encode_amount(start_balance),
        )
    )

    # Transactions (04 / 05)
    seq = 1
    for tx in transactions:
        ref = f"{seq:07d}"
        label = tx["label"]
        name = label[:31]
        amount_str = cfonb_encode_amount(tx["amount"])
        lines.append(
            build_line(
                "04",
                bank_code,
                branch_code,
                account_number,
                date_ddmmyy=tx["date"].strftime("%d%m%y"),
                amount_str=amount_str,
                name=name,
                ref=ref,
            )
        )
        if len(label) > 31:
            comp = label[31:31 + 70]
            lines.append(
                build_line(
                    "05",
                    bank_code,
                    branch_code,
                    account_number,
                    date_ddmmyy=tx["date"].strftime("%d%m%y"),
                    comp_type="LIB",
                    comp_info=comp,
                )
            )
        seq += 1

    # Closing (07)
    lines.append(
        build_line(
            "07",
            bank_code,
            branch_code,
            account_number,
            date_ddmmyy=end_date.strftime("%d%m%y"),
            amount_str=cfonb_encode_amount(end_balance),
        )
    )

    with open(args.output_cfo, "w", encoding="latin1", newline="\n") as f:
        for line in lines:
            if len(line) != CFONB_WIDTH:
                raise ValueError(f"Line length is {len(line)} instead of {CFONB_WIDTH}")
            f.write(line + "\n")

    print(f"CFONB120 file written to: {args.output_cfo}")

if __name__ == "__main__":
    main()