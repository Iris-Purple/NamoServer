import re

class ProtoParser():
    def __init__(self, start_id, recv_prefix, send_prefix):
        self.recv_pkt = []      # 수신 패킷 목록
        self.send_pkt = []      # 송신 패킷 목록
        self.total_pkt = []     # 모든 패킷 목록
        self.start_id = start_id
        self.id = start_id
        self.recv_prefix = recv_prefix
        self.send_prefix = send_prefix
        self.packet_id_map = {} # enum PacketId에서 파싱한 ID 매핑

    def parse_proto(self, path):
        f = open(path, 'r')
        content = f.read()
        f.close()

        # 1. enum PacketId 블록에서 ID 파싱
        self._parse_packet_id_enum(content)

        # 2. message 파싱
        lines = content.split('\n')
        for line in lines:
            if line.startswith('message') == False:
                continue

            pkt_name = line.split()[1].upper()
            
            # enum에서 ID 찾기, 없으면 자동 증가 ID 사용
            pkt_id = self.packet_id_map.get(pkt_name, self.id)
            
            if pkt_name.startswith(self.recv_prefix):
                self.recv_pkt.append(Packet(pkt_name, pkt_id))
            elif pkt_name.startswith(self.send_prefix):
                self.send_pkt.append(Packet(pkt_name, pkt_id))
            else:
                continue

            self.total_pkt.append(Packet(pkt_name, pkt_id))
            
            # enum에 없는 경우에만 자동 증가
            if pkt_name not in self.packet_id_map:
                self.id += 1

    def _parse_packet_id_enum(self, content):
        """enum PacketId { PKT_C2S_XXX = 2000; } 파싱"""
        # enum PacketId 블록 찾기
        enum_match = re.search(r'enum\s+PacketId\s*\{([^}]+)\}', content)
        if not enum_match:
            return
        
        enum_body = enum_match.group(1)
        
        # PKT_C2S_ENTER_GAME = 2000; 형태 파싱
        for line in enum_body.split('\n'):
            match = re.match(r'\s*PKT_(\w+)\s*=\s*(\d+)', line)
            if match:
                pkt_name = match.group(1).upper()  # C2S_ENTER_GAME
                pkt_id = int(match.group(2))       # 2000
                self.packet_id_map[pkt_name] = pkt_id


class Packet:
    def __init__(self, name, id):
        self.name = name
        self.id = id