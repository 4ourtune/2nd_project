import sys
import can
import time
from PyQt5 import QtWidgets, QtCore, QtGui


class ECUInfoDialog(QtWidgets.QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("✏️ Edit ECU Info (WriteDataByIdentifier 0x2E)")
        self.setFixedSize(400, 320)
        layout = QtWidgets.QFormLayout(self)

        # 기본 값 (C 구조체 초기값과 동일)
        self.vin_edit = QtWidgets.QLineEdit("MY_TC375_VIN_001")
        self.hw_edit = QtWidgets.QLineEdit("TC375_HW_V1.0.0")
        self.sw_edit = QtWidgets.QLineEdit("MY_APP_SW_V1.2.3")
        self.sn_edit = QtWidgets.QLineEdit("SN_ECU_1234567890")
        self.supplier_edit = QtWidgets.QLineEdit("MyProjectSupplier")

        layout.addRow("VIN:", self.vin_edit)
        layout.addRow("Hardware PN:", self.hw_edit)
        layout.addRow("Software PN:", self.sw_edit)
        layout.addRow("Serial Number:", self.sn_edit)
        layout.addRow("Supplier:", self.supplier_edit)

        self.btn_send = QtWidgets.QPushButton("🚀 Send to ECU (0x2E)")
        layout.addRow(self.btn_send)
        self.btn_send.clicked.connect(self.accept)

    def get_info_bytes(self):
        def to_fixed_bytes(s, length):
            b = s.encode('ascii', errors='ignore')[:length]
            return b + b'\x00' * (length - len(b))

        vin = to_fixed_bytes(self.vin_edit.text(), 18)
        hw = to_fixed_bytes(self.hw_edit.text(), 20)
        sw = to_fixed_bytes(self.sw_edit.text(), 20)
        sn = to_fixed_bytes(self.sn_edit.text(), 20)
        supplier = to_fixed_bytes(self.supplier_edit.text(), 20)
        return vin + hw + sw + sn + supplier


class CANUDSGui(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("CAN/UDS Diagnostic GUI")
        self.resize(1150, 820)

        # 밝은 테마
        self.setStyleSheet("""
            QWidget { background-color: #FAFAFF; color: #111; font-family: 'Segoe UI'; font-size: 10pt; }
            QPushButton { background-color: #E9F1FF; border: 1px solid #BBD1FF; border-radius: 6px; padding: 6px; }
            QPushButton:hover { background-color: #D9E9FF; }
            QTextEdit { background-color: #FFFFFF; border: 1px solid #CFCFCF; border-radius: 4px; font-family: Consolas; }
            QTableWidget { background-color: #FFFFFF; border: 1px solid #CFCFCF; border-radius: 4px; }
            QGroupBox { font-weight: 600; }
            QLabel.title { font-size: 12pt; font-weight: 600; }
        """)

        # 기본 설정 (PCAN USB)
        self.channel = "PCAN_USBBUS1"
        self.bustype = "pcan"
        self.bitrate = 500000
        self.req_id = 0x7E0
        self.res_id = 0x7E8

        root = QtWidgets.QVBoxLayout(self)

        # UDS 버튼
        uds_row = QtWidgets.QHBoxLayout()
        self.btn_read_ecu_info = QtWidgets.QPushButton("ℹ️ Read ECU Info (DID 0x0005)")
        self.btn_write_ecu_info = QtWidgets.QPushButton("✏️ Write ECU Info (0x2E 0005)")
        self.btn_read_dtc = QtWidgets.QPushButton("📖 Read DTCs (0x19)")
        self.btn_clear_dtc = QtWidgets.QPushButton("🧹 Clear DTCs (0x14)")
        uds_row.addWidget(self.btn_read_ecu_info)
        uds_row.addWidget(self.btn_write_ecu_info)
        uds_row.addWidget(self.btn_read_dtc)
        uds_row.addWidget(self.btn_clear_dtc)
        root.addLayout(uds_row)

        # 센서 버튼
        sensor_row = QtWidgets.QHBoxLayout()
        self.btn_ultra1 = QtWidgets.QPushButton("🔹 Ultra Sensor 1 (0x0001)")
        self.btn_ultra2 = QtWidgets.QPushButton("🔹 Ultra Sensor 2 (0x0002)")
        self.btn_ultra3 = QtWidgets.QPushButton("🔹 Ultra Sensor 3 (0x0003)")
        self.btn_tof = QtWidgets.QPushButton("🔹 ToF Sensor (0x0004)")
        sensor_row.addWidget(self.btn_ultra1)
        sensor_row.addWidget(self.btn_ultra2)
        sensor_row.addWidget(self.btn_ultra3)
        sensor_row.addWidget(self.btn_tof)
        root.addLayout(sensor_row)

        # 송수신 프레임 모니터
        frame_layout = QtWidgets.QHBoxLayout()
        self.sent_box = QtWidgets.QTextEdit(); self.sent_box.setReadOnly(True)
        self.recv_box = QtWidgets.QTextEdit(); self.recv_box.setReadOnly(True)
        left = QtWidgets.QVBoxLayout(); left.addWidget(QtWidgets.QLabel("📤 Sent Frames")); left.addWidget(self.sent_box)
        right = QtWidgets.QVBoxLayout(); right.addWidget(QtWidgets.QLabel("📥 Received Frames")); right.addWidget(self.recv_box)
        frame_layout.addLayout(left); frame_layout.addLayout(right)
        root.addLayout(frame_layout)

        # ECU Info 카드
        info_card = QtWidgets.QGroupBox("🧾 ECU Information")
        info_layout = QtWidgets.QFormLayout()
        self.lbl_vin = QtWidgets.QLabel("-")
        self.lbl_hw = QtWidgets.QLabel("-")
        self.lbl_sw = QtWidgets.QLabel("-")
        self.lbl_sn = QtWidgets.QLabel("-")
        self.lbl_supplier = QtWidgets.QLabel("-")
        info_layout.addRow("VIN:", self.lbl_vin)
        info_layout.addRow("HW:", self.lbl_hw)
        info_layout.addRow("SW:", self.lbl_sw)
        info_layout.addRow("SN:", self.lbl_sn)
        info_layout.addRow("Supplier:", self.lbl_supplier)
        info_card.setLayout(info_layout)
        root.addWidget(info_card)

        # 센서 결과
        sensor_card = QtWidgets.QGroupBox("📡 Sensor Values")
        self.sensor_result = QtWidgets.QTextEdit(); self.sensor_result.setReadOnly(True)
        scv = QtWidgets.QVBoxLayout(); scv.addWidget(self.sensor_result); sensor_card.setLayout(scv)
        root.addWidget(sensor_card)

        # DTC 테이블
        dtc_card = QtWidgets.QGroupBox("⚙️ Diagnostic Trouble Codes (DTC)")
        dtc_layout = QtWidgets.QVBoxLayout()
        self.dtc_table = QtWidgets.QTableWidget(0, 4)
        self.dtc_table.setHorizontalHeaderLabels(["#", "DTC 코드", "상태값", "상태 설명"])
        self.dtc_table.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.Stretch)
        dtc_layout.addWidget(self.dtc_table)
        dtc_card.setLayout(dtc_layout)
        root.addWidget(dtc_card)

        # 로그, 결과
        self.log_box = QtWidgets.QTextEdit(); self.log_box.setReadOnly(True)
        self.result_box = QtWidgets.QTextEdit(); self.result_box.setReadOnly(True)
        root.addWidget(QtWidgets.QLabel("🪶 Log Output"))
        root.addWidget(self.log_box)
        root.addWidget(QtWidgets.QLabel("📊 Diagnostic Results"))
        root.addWidget(self.result_box)

        # CAN 초기화
        try:
            self.bus = can.interface.Bus(channel=self.channel, bustype=self.bustype, bitrate=self.bitrate)
            self.log("✅ PCAN USB connected successfully.")
        except Exception as e:
            self.bus = None
            self.log(f"❌ CAN init failed: {e}")

        # 버튼 연결
        self.btn_ultra1.clicked.connect(lambda: self.read_by_did(0x0001))
        self.btn_ultra2.clicked.connect(lambda: self.read_by_did(0x0002))
        self.btn_ultra3.clicked.connect(lambda: self.read_by_did(0x0003))
        self.btn_tof.clicked.connect(lambda: self.read_by_did(0x0004))
        self.btn_read_ecu_info.clicked.connect(lambda: self.read_by_did(0x0005))
        self.btn_write_ecu_info.clicked.connect(self.write_ecu_info)
        self.btn_read_dtc.clicked.connect(self.read_dtc)
        self.btn_clear_dtc.clicked.connect(self.clear_dtc)

    # ---------------- 유틸 ----------------
    def log(self, text):
        self.log_box.append(text)
        self.log_box.verticalScrollBar().setValue(self.log_box.verticalScrollBar().maximum())

    def recv_all(self, timeout=2.0):
        frames = []
        start = time.time()
        while time.time() - start < timeout:
            msg = self.bus.recv(timeout=0.1) if self.bus else None
            if msg and msg.arbitration_id == self.res_id:
                frames.append(msg)
        return frames

    def show_frames(self, frames):
        for f in frames:
            self.recv_box.append(f"ID:{hex(f.arbitration_id)} DATA: {' '.join(f'{b:02X}' for b in f.data)}")
        self.recv_box.verticalScrollBar().setValue(self.recv_box.verticalScrollBar().maximum())

    # ---------------- 기능: 0x22 ----------------
    def read_by_did(self, did):
        if not self.bus:
            self.log("⚠️ CAN bus not ready")
            return

        did_h = (did >> 8) & 0xFF
        did_l = did & 0xFF
        tx = [0x03, 0x22, did_h, did_l, 0, 0, 0, 0]
        msg = can.Message(arbitration_id=self.req_id, data=tx, is_extended_id=False)
        self.bus.send(msg)
        self.sent_box.append(" ".join(f"{b:02X}" for b in tx))
        self.log(f"▶ Sent ReadDataByIdentifier DID=0x{did:04X}")

        frames = self.recv_all(timeout=3.0)
        if frames:
            self.show_frames(frames)
            self.parse_uds_response(frames, 0x22, did)

    # ---------------- 기능: 0x2E ----------------
    def write_ecu_info(self):
        dialog = ECUInfoDialog(self)
        if dialog.exec_():
            data = dialog.get_info_bytes()
            did_h, did_l = 0x00, 0x05
            uds_data = bytearray([0x2E, did_h, did_l]) + data
            total_len = len(uds_data)
            self.log(f"🚀 Sending {total_len} bytes (ECU Info) via TP...")

            # First Frame
            ff = bytearray(8)
            ff[0] = 0x10 | ((total_len >> 8) & 0x0F)
            ff[1] = total_len & 0xFF
            ff[2:8] = uds_data[:6]
            self.bus.send(can.Message(arbitration_id=self.req_id, data=ff, is_extended_id=False))
            self.sent_box.append(" ".join(f"{b:02X}" for b in ff))
            time.sleep(0.01)

            sent = 6
            sn = 1
            while sent < total_len:
                cf = bytearray(8)
                cf[0] = 0x20 | (sn & 0x0F)
                for i in range(1, 8):
                    if sent < total_len:
                        cf[i] = uds_data[sent]
                        sent += 1
                self.bus.send(can.Message(arbitration_id=self.req_id, data=cf, is_extended_id=False))
                self.sent_box.append(" ".join(f"{b:02X}" for b in cf))
                sn = (sn + 1) % 16
                time.sleep(0.01)

            self.log("✅ ECU Info write request sent successfully!")

    # ---------------- 기능: 0x19 ----------------
    def read_dtc(self):
        tx = [0x03, 0x19, 0x02, 0xFF, 0, 0, 0, 0]
        msg = can.Message(arbitration_id=self.req_id, data=tx, is_extended_id=False)
        self.bus.send(msg)
        self.sent_box.append(" ".join(f"{b:02X}" for b in tx))
        self.log("▶ Sent ReadDTCInformation")

        frames = self.recv_all(timeout=3.0)
        if frames:
            self.show_frames(frames)
            self.parse_uds_response(frames, 0x19, 0)

    # ---------------- 기능: 0x14 ----------------
    def clear_dtc(self):
        tx = [0x02, 0x14, 0xFF, 0, 0, 0, 0, 0]
        msg = can.Message(arbitration_id=self.req_id, data=tx, is_extended_id=False)
        self.bus.send(msg)
        self.sent_box.append(" ".join(f"{b:02X}" for b in tx))
        self.log("▶ Sent ClearDiagnosticInformation")
        frames = self.recv_all(timeout=2.0)
        if frames:
            self.show_frames(frames)
            self.log("✅ DTC Clear acknowledged (0x54).")

    # ---------------- TP 파서 (프로급, 서비스별 포맷 분리) ----------------
    def parse_uds_response(self, frames, req_sid, did):
        """
        - Single/First/Consecutive/FlowControl frame 자동 조합
        - 서비스(0x22/0x2E vs 0x19/0x14)에 따라 헤더 분리 방식 다르게 적용
        """
        payload = bytearray()
        total_len = 0
        sn_expected = 1
        pos_sid = (req_sid + 0x40) & 0xFF

        got_ff = False

        for f in frames:
            d = f.data
            pci_type = d[0] >> 4

            if pci_type == 3:  # FC
                fs = d[0] & 0x0F
                block_size = d[1]
                stmin = d[2]
                self.log(f"🌀 FlowControl Frame: FS={fs} BS={block_size} STmin={stmin}")
                continue

            if pci_type == 0:  # SF
                sf_len = d[0] & 0x0F
                payload.extend(d[1:1+sf_len])
                self.log(f"📥 Single Frame received ({sf_len} bytes).")
                continue

            if pci_type == 1:  # FF
                total_len = ((d[0] & 0x0F) << 8) | d[1]
                payload.extend(d[2:8])
                sn_expected = 1
                got_ff = True
                self.log(f"📦 First Frame (len={total_len}) received.")
                continue

            if pci_type == 2:  # CF
                sn = d[0] & 0x0F
                if sn != (sn_expected & 0x0F):
                    self.log(f"⚠️ SeqErr: expected SN={sn_expected}, got={sn}")
                sn_expected = (sn_expected + 1) % 16
                payload.extend(d[1:])
                continue

        self.log(f"ℹ️ Reconstructed payload ({len(payload)}/{total_len if got_ff else 'SF'} bytes): "
                 f"{' '.join(f'{b:02X}' for b in payload[:40])}{' ...' if len(payload) > 40 else ''}")

        # ... 프레임 루프 후
        if not payload:
            # 폴백: 어떤 ECU는 ISO-TP 없이 0x59 ... 을 바로 보낼 수 있음
            # 이 경우 첫 프레임의 데이터로 간주해서 최소 판정만 수행
            if frames and len(frames[0].data) >= 2 and frames[0].data[0] == (req_sid + 0x40) & 0xFF:
                raw = bytes(frames[0].data)
                # 0x59 0x02 [0x00 or 생략] 형태를 'DTC 없음'으로 간주
                if req_sid == 0x19:
                    # mask가 없거나 0x00이면 없음 처리
                    if len(raw) == 2 or (len(raw) >= 3 and raw[2] == 0x00):
                        self.update_dtc_table([])
                        self.result_box.append("✅ DTC 없음 (fallback)")
                        return
            self.log("⚠️ No valid UDS payload collected.")
            return

        uds_sid = payload[0]
        if uds_sid != pos_sid:
            self.log(f"⚠️ Unexpected SID=0x{uds_sid:02X} (expected 0x{pos_sid:02X})")
            return

        # -------- 헤더 분리 (서비스별 상이) --------
        if req_sid in (0x22, 0x2E):
            # [POS SID][DID_H][DID_L] ...
            if len(payload) < 3:
                self.log("⚠️ Payload too short for DID-based response.")
                return
            uds_did = (payload[1] << 8) | payload[2]
            data = payload[3:]
            data_len = len(data)

            # ECU Info
            if did == 0x0005 and data_len >= 98:
                def safe_decode(segment):
                    try:
                        return segment.split(b'\x00')[0].decode('ascii', errors='ignore')
                    except Exception:
                        return "<DecodeError>"

                vin = safe_decode(data[0:18])
                hw = safe_decode(data[18:38])
                sw = safe_decode(data[38:58])
                sn = safe_decode(data[58:78])
                supplier = safe_decode(data[78:98])

                self.lbl_vin.setText(vin)
                self.lbl_hw.setText(hw)
                self.lbl_sw.setText(sw)
                self.lbl_sn.setText(sn)
                self.lbl_supplier.setText(supplier)
                self.result_box.append("✅ ECU Info Updated")
                self.result_box.append(f" VIN: {vin}")
                self.result_box.append(f" HW : {hw}")
                self.result_box.append(f" SW : {sw}")
                self.result_box.append(f" SN : {sn}")
                self.result_box.append(f" Supplier: {supplier}")
                self.log("📗 ECU Info successfully parsed (98 bytes).")
                return

            # 센서 (2바이트 값)
            if did in (0x0001, 0x0002, 0x0003, 0x0004):
                if data_len >= 3:
                    val = (data[0] << 8) | data[1]
                    status_char = chr(data[2]) if 32 <= data[2] <= 126 else '?'
                    self.sensor_result.append(
                        f"✅ Sensor DID 0x{did:04X}: {val} mm (0x{val:04X}) '{status_char}'"
                    )
                    self.log(f"📘 Sensor data decoded successfully (Result={status_char}).")
                elif data_len >= 2:
                    val = (data[0] << 8) | data[1]
                    self.sensor_result.append(
                        f"✅ Sensor DID 0x{did:04X}: {val} mm (0x{val:04X})"
                    )
                    self.log("📘 Sensor data decoded (no P/F byte).")
                else:
                    self.log(f"⚠️ Invalid sensor payload length ({data_len}).")
                return

            # Unknown DID
            printable = all(32 <= b < 127 for b in data)
            if printable:
                text = data.split(b'\x00')[0].decode('ascii', errors='ignore')
                self.result_box.append(f"🟡 Raw Text Data: {text}")
            else:
                self.result_box.append(f"🟠 Raw Binary Data ({data_len} bytes): "
                                       f"{' '.join(f'{b:02X}' for b in data[:20])}...")
            self.log(f"ℹ️ Unknown DID=0x{uds_did:04X}, len={data_len}.")
            return

        elif req_sid == 0x19:
            # [POS SID][subfunc][optional ...]
            if len(payload) < 2:
                self.log("⚠️ DTC payload too short.")
                return
            subfunc = payload[1]
            data = payload[2:]  # status mask + entries...
            data_len = len(data)

            # DTC 없음: 마스크만 있고 항목 0개여도(= 길이 1) 없음으로 간주
            if data_len <= 1:
                # data_len == 0  → 아예 데이터 없음
                # data_len == 1  → [availability_mask]만 존재 (예: 0xFF). 항목은 0개.
                self.result_box.append("✅ DTC 없음 (No Diagnostic Trouble Code Stored)")
                self.update_dtc_table([])
                self.log("📙 DTC information decoded (empty via availability mask only).")
                return

            # ✅ DTC 있음: [statusMask][(DTC hi, mid, lo, status)*]
            if data_len >= 5:
                status_mask = data[0]
                dtcs = []
                off = 1
                while off + 3 < data_len:
                    code = (data[off] << 16) | (data[off+1] << 8) | data[off+2]
                    status = data[off+3]
                    dtcs.append((code, status))
                    off += 4

                self.result_box.append(f"✅ DTC Count: {len(dtcs)}")
                for i, (code, status) in enumerate(dtcs, 1):
                    desc = self.get_dtc_status_desc(status)
                    self.result_box.append(f" DTC {i}: 0x{code:06X}, Status: 0x{status:02X} ({desc})")

                self.update_dtc_table(dtcs)
                self.log("📙 DTC information decoded.")
                return

            self.log("⚠️ DTC payload format not recognized.")
            return

        elif req_sid == 0x14:
            # ClearDTC positive: [0x54][0xFF] ...
            self.result_box.append("✅ DTC Clear acknowledged (0x54).")
            self.update_dtc_table([])   # ★ 테이블 즉시 비우기
            return

        else:
            self.log(f"ℹ️ Unhandled Service 0x{req_sid:02X}.")

    # ---------------- DTC 테이블 업데이트 ----------------
    def update_dtc_table(self, dtcs):
        self.dtc_table.setRowCount(0)
        if not dtcs:
            self.dtc_table.insertRow(0)
            self.dtc_table.setItem(0, 0, QtWidgets.QTableWidgetItem("1"))
            self.dtc_table.setItem(0, 1, QtWidgets.QTableWidgetItem("없음"))
            self.dtc_table.setItem(0, 2, QtWidgets.QTableWidgetItem("-"))
            self.dtc_table.setItem(0, 3, QtWidgets.QTableWidgetItem("DTC 없음"))
            return

        for i, (code, status) in enumerate(dtcs, 1):
            row = self.dtc_table.rowCount()
            self.dtc_table.insertRow(row)
            self.dtc_table.setItem(row, 0, QtWidgets.QTableWidgetItem(str(i)))
            self.dtc_table.setItem(row, 1, QtWidgets.QTableWidgetItem(f"0x{code:06X}"))
            self.dtc_table.setItem(row, 2, QtWidgets.QTableWidgetItem(f"0x{status:02X}"))
            self.dtc_table.setItem(row, 3, QtWidgets.QTableWidgetItem(self.get_dtc_status_desc(status)))

    # ---------------- 상태 설명 변환 ----------------
    def get_dtc_status_desc(self, status):
        mapping = {
            0x00: "정상 (No Fault)",
            0x01: "보류 (Pending)",
            0x08: "이번 주기 실패",
            0x10: "현재 실패 (Test Failed)",
            0x40: "확정 (Confirmed)",
        }
        return mapping.get(status, f"알 수 없음 (0x{status:02X})")


if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    gui = CANUDSGui()
    gui.show()
    sys.exit(app.exec_())
