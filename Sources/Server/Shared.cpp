/*
 Copyright (c) 2013 yvt
 based on code of pysnip (c) Mathias Kaerlev 2011-2012.
 
 This file is part of OpenSpades.
 
 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.
 
 */

#include "Shared.h"
#include <Core/ENetTools.h>
#include <Core/Debug.h>
#include <Core/VersionInfo.h>
#include <OpenSpades.h>
#include <Core//Settings.h>

SPADES_SETTING(core_locale, "");

namespace spades { namespace protocol {
	
	typedef Packet *(*PacketDecodeFuncType)(const std::vector<char>&);
	
	class PacketTypeFinder {
		PacketType type;
	public:
		constexpr PacketTypeFinder(PacketType type): type(type) {}
		template<class T> inline constexpr bool evaluate() const { return type == T::Type; }
	};
	class PacketTypeToDecoder {
	public:
		template<class T> inline constexpr PacketDecodeFuncType evaluate() const { return &T::Decode; }
		inline constexpr PacketDecodeFuncType not_found() const { return nullptr; }
	};
	
	
	class PacketDecodeTableGenerator {
	public:
		constexpr PacketDecodeFuncType operator [](std::size_t index) const {
			return stmp::find_type_list<PacketTypeFinder, PacketTypeToDecoder, PacketClassList>
			(PacketTypeFinder(static_cast<PacketType>(index)), PacketTypeToDecoder()).evaluate();
		}
	};
	
	static constexpr auto packetDecodeTable = stmp::make_static_table<128>(PacketDecodeTableGenerator());
	
	class PacketReader: public NetPacketReader {
	public:
		PacketReader(const std::vector<char>& bytes):
		NetPacketReader(bytes) {}
		
		uint64_t ReadVariableInteger() {
			SPADES_MARK_FUNCTION();
			
			uint32_t v = 0;
			int shift = 0;
			while(true) {
				uint8_t b = ReadByte();
				v |= static_cast<uint32_t>(b & 0x7f) << shift;
				if(b & 0x80) {
					shift += 7;
				}else{
					break;
				}
			}
			return v;
		}
		
		int64_t ReadVariableSignedInteger() {
			auto i = ReadVariableInteger();
			if(i & 1) {
				return -1 - static_cast<int64_t>(i>>1);
			}else{
				return static_cast<int64_t>(i >> 1);
			}
		}
		
		std::string ReadBytes(){
			SPADES_MARK_FUNCTION();
			
			auto len = ReadVariableInteger();
			if(len > 1024 * 1024) {
				SPRaise("String too long.: %llu",
						static_cast<unsigned long long>(len));
			}
			
			// convert to C string once so that
			// null-chars are removed
			std::string s = ReadData(static_cast<std::size_t>(len)).c_str();
			return s;
		}
		
		std::string ReadString(){
			return ReadBytes();
		}
		
		template<class T>
		T ReadMap() {
			T dict;
			while(true){
				auto key = ReadString();
				if(key.empty()) break;
				auto value = ReadString();
				dict.insert(std::make_pair(key, value));
			}
			return std::move(dict);
		}
		
		TimeStampType ReadTimeStamp() {
			return static_cast<TimeStampType>(ReadVariableInteger());
		}
		
		IntVector3 ReadBlockCoord() {
			auto x = static_cast<int>(ReadVariableSignedInteger());
			auto y = static_cast<int>(ReadVariableSignedInteger());
			auto z = static_cast<int>(ReadVariableSignedInteger());
			return IntVector3(x, y, z);
		}
		
	};
	class PacketWriter: public NetPacketWriter {
	public:
		PacketWriter(PacketType type):
		NetPacketWriter(static_cast<unsigned int>(type)) {}
		
		void WriteVariableInteger(uint64_t i) {
			SPADES_MARK_FUNCTION();
			
			while(true) {
				uint8_t b = static_cast<uint8_t>(i & 0x7f);
				i >>= 7;
				if(i) {
					b |= 0x80;
					Write(b);
				}else{
					Write(b);
					break;
				}
			}
		}
		
		void WriteVariableSignedInteger(int64_t i) {
			if(i < 0) WriteVariableInteger((static_cast<uint64_t>(-1 - i) << 1) | 1);
			else WriteVariableInteger(static_cast<uint64_t>(i) << 1);
		}
		
		void WriteBytes(std::string str){
			SPADES_MARK_FUNCTION();
			
			WriteVariableInteger(str.size());
			Write(str);
		}
		void WriteString(std::string str){
			WriteBytes(str);
		}
		template<class T>
		void WriteMap(const T& dict) {
			for(const auto& item: dict) {
				if(item.first.empty()) continue;
				WriteString(item.first);
				WriteString(item.second);
			}
			WriteString(std::string());
		}
		
		using NetPacketWriter::Write;
		void Write(TimeStampType t) {
			WriteVariableInteger(static_cast<uint64_t>(t));
		}
		
		void WriteBlockCoord(IntVector3 v) {
			WriteVariableSignedInteger(static_cast<uint64_t>(v.x));
			WriteVariableSignedInteger(static_cast<uint64_t>(v.y));
			WriteVariableSignedInteger(static_cast<uint64_t>(v.z));
		}
	};
	
	Packet *Packet::Decode(const std::vector<char>& data) {
		SPADES_MARK_FUNCTION();
		
		if(data.size() == 0) {
			SPRaise("Packet truncated");
		}
		
		auto typeIndex = static_cast<std::size_t>(data[0]);
		if(typeIndex >= packetDecodeTable.size()) {
			return nullptr;
		}
		
		auto *ptr = packetDecodeTable[typeIndex];
		if(ptr == nullptr) {
			return nullptr;
		}
		
		return ptr(data);
	}
	
	
	Packet *GreetingPacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<GreetingPacket> p(new GreetingPacket());
		PacketReader reader(data);
		
		auto magic = reader.ReadString();
		if(magic != "Hello") {
			SPRaise("Invalid magic.");
		}
		p->nonce = reader.ReadBytes();
		
		return p.release();
	}
	
	std::vector<char> GreetingPacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.WriteString("Hello");
		writer.WriteBytes(nonce);
		
		return std::move(writer.ToArray());
	}
	
	
	InitiateConnectionPacket InitiateConnectionPacket::CreateDefault() {
		SPADES_MARK_FUNCTION();
		
		InitiateConnectionPacket ret;
		ret.protocolName = ProtocolName;
		if(ret.protocolName.size() > 256) ret.protocolName.resize(256);
		ret.majorVersion = OpenSpades_VERSION_MAJOR;
		ret.minorVersion = OpenSpades_VERSION_MINOR;
		ret.revision = OpenSpades_VERSION_REVISION;
		ret.packageString = PACKAGE_STRING;
		if(ret.packageString.size() > 256) ret.packageString.resize(256);
		ret.environmentString = VersionInfo::GetVersionInfo();
		if(ret.environmentString.size() > 1024) ret.environmentString.resize(1024);
		ret.locale = static_cast<std::string>(core_locale);
		if(ret.locale.size() > 256) ret.locale.resize(256);
		
		return ret;
	}
	
	Packet *InitiateConnectionPacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<InitiateConnectionPacket> p(new InitiateConnectionPacket());
		PacketReader reader(data);
		
		p->protocolName = reader.ReadString();
		p->majorVersion = reader.ReadShort();
		p->minorVersion = reader.ReadShort();
		p->revision = reader.ReadShort();
		p->packageString = reader.ReadString();
		p->environmentString = reader.ReadString();
		p->locale = reader.ReadString();
		p->playerName = reader.ReadString();
		p->nonce = reader.ReadBytes();
		
		return p.release();
	}
	
	std::vector<char> InitiateConnectionPacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.WriteString(protocolName);
		writer.Write(majorVersion);
		writer.Write(minorVersion);
		writer.Write(revision);
		writer.WriteString(packageString);
		writer.WriteString(environmentString);
		writer.WriteString(locale);
		writer.WriteString(playerName);
		writer.WriteBytes(nonce);
		
		return std::move(writer.ToArray());
	}
	
	
	Packet *ServerCertificatePacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<ServerCertificatePacket> p(new ServerCertificatePacket());
		PacketReader reader(data);
		
		p->isValid = reader.ReadByte() != 0;
		
		if(p->isValid) {
			p->certificate = reader.ReadBytes();
			p->signature = reader.ReadBytes();
		}
		
		return p.release();
	}
	
	std::vector<char> ServerCertificatePacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.Write(static_cast<uint8_t>(isValid ? 1 : 0));
		if(isValid) {
			writer.WriteBytes(certificate);
			writer.WriteBytes(signature);
		}
			
		return std::move(writer.ToArray());
	}
	
	
	Packet *ClientCertificatePacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<ClientCertificatePacket> p(new ClientCertificatePacket());
		PacketReader reader(data);
		
		p->isValid = reader.ReadByte() != 0;
		
		if(p->isValid) {
			p->certificate = reader.ReadBytes();
			p->signature = reader.ReadBytes();
		}
		
		return p.release();
	}
	
	std::vector<char> ClientCertificatePacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.Write(static_cast<uint8_t>(isValid ? 1 : 0));
		if(isValid) {
			writer.WriteBytes(certificate);
			writer.WriteBytes(signature);
		}
		
		return std::move(writer.ToArray());
	}
	
	
	Packet *KickPacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<KickPacket> p(new KickPacket());
		PacketReader reader(data);
		
		p->reason = reader.ReadString();
		
		return p.release();
	}
	
	std::vector<char> KickPacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.WriteString(reason);
		
		return std::move(writer.ToArray());
	}
	
	Packet *GameStateHeaderPacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<GameStateHeaderPacket> p(new GameStateHeaderPacket());
		PacketReader reader(data);
		
		p->properties = reader.ReadMap<std::map<std::string, std::string>>();
		
		return p.release();
	}
	
	std::vector<char> GameStateHeaderPacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.WriteMap(properties);
		
		return std::move(writer.ToArray());
	}

	
	
	Packet *MapDataPacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<MapDataPacket> p(new MapDataPacket());
		PacketReader reader(data);
		
		p->fragment = reader.ReadBytes();
		
		return p.release();
	}
	
	std::vector<char> MapDataPacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.WriteBytes(fragment);
		
		return std::move(writer.ToArray());
	}
	
	
	Packet *GameStateFinalPacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<GameStateFinalPacket> p(new GameStateFinalPacket());
		PacketReader reader(data);
		
		p->properties = reader.ReadMap<std::map<std::string, std::string>>();
		
		return p.release();
	}
	
	std::vector<char> GameStateFinalPacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.WriteMap(properties);
		
		return std::move(writer.ToArray());
	}
	
	
	Packet *GenericCommandPacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<GenericCommandPacket> p(new GenericCommandPacket());
		PacketReader reader(data);
		
		while(!reader.IsEndOfPacket()) {
			p->parts.push_back(reader.ReadBytes());
		}
		
		return p.release();
	}
	
	std::vector<char> GenericCommandPacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		for(const auto& part: parts) {
			writer.WriteBytes(part);
		}
		
		return std::move(writer.ToArray());
	}
	
	enum class EntityUpdateFlags {
		None = 0,
		Create = 1 << 0,
		Flags = 1 << 1,
		Trajectory = 1 << 2,
		PlayerInput = 1 << 3,
		Tool = 1 << 4,
		BlockColor = 1 << 5,
		Health = 1 << 6,
		Skins = 1 << 7
	};
	
	inline EntityUpdateFlags operator|(EntityUpdateFlags a, EntityUpdateFlags b)
	{
		return static_cast<EntityUpdateFlags>(static_cast<int>(a) | static_cast<int>(b));
	}
	inline EntityUpdateFlags& operator |=(EntityUpdateFlags& a, EntityUpdateFlags b) {
		a = a | b;
		return a;
	}
	inline bool operator&(EntityUpdateFlags a, EntityUpdateFlags b)
	{
		return static_cast<int>(a) & static_cast<int>(b);
	}
	
	enum class EntityFlagsValue {
		None = 0,
		PlayerClip = 1 << 0,
		WeaponClip = 1 << 1,
		Fly = 1 << 2
	};
	
	inline EntityFlagsValue operator|(EntityFlagsValue a, EntityFlagsValue b)
	{
		return static_cast<EntityFlagsValue>(static_cast<int>(a) | static_cast<int>(b));
	}
	inline EntityFlagsValue& operator |=(EntityFlagsValue& a, EntityFlagsValue b) {
		a = a | b;
		return a;
	}
	inline bool operator&(EntityFlagsValue a, EntityFlagsValue b)
	{
		return static_cast<int>(a) & static_cast<int>(b);
	}
	
	inline EntityFlagsValue ToEntityFlagsValue(EntityFlags flags) {
		auto ret = EntityFlagsValue::None;
		if(flags.playerClip) ret |= EntityFlagsValue::PlayerClip;
		if(flags.weaponClip) ret |= EntityFlagsValue::WeaponClip;
		if(flags.fly) ret |= EntityFlagsValue::Fly;
		return ret;
	}
	
	inline EntityFlags FromEntityFlagsValue(EntityFlagsValue val) {
		EntityFlags ret;
		ret.playerClip = val & EntityFlagsValue::PlayerClip;
		ret.weaponClip = val & EntityFlagsValue::WeaponClip;
		ret.fly = val & EntityFlagsValue::Fly;
		return ret;
	}
	
	static Trajectory DecodeTrajectory(PacketReader& reader) {
		Trajectory traj;
		traj.type = static_cast<TrajectoryType>(reader.ReadByte());
		traj.origin = reader.ReadVector3();
		traj.velocity = reader.ReadVector3();
		
		switch(traj.type) {
			case game::TrajectoryType::Linear:
			case game::TrajectoryType::Gravity:
			case game::TrajectoryType::Constant:
			case game::TrajectoryType::RigidBody:
				traj.angle = Quaternion::DecodeRotation(reader.ReadVector3());
				traj.angularVelocity = reader.ReadVector3();
				break;
			case game::TrajectoryType::Player:
				traj.eulerAngle = reader.ReadVector3();
				break;
			default:
				SPRaise("Unknown trajectory type: %d",
						static_cast<int>(traj.type));
		}
		return traj;
	}
	
	static void WriteTrajectory(PacketWriter& writer, const Trajectory& traj) {
		writer.Write(static_cast<uint8_t>(traj.type));
		writer.Write(traj.origin);
		writer.Write(traj.velocity);
		switch(traj.type) {
			case game::TrajectoryType::Linear:
			case game::TrajectoryType::Gravity:
			case game::TrajectoryType::Constant:
			case game::TrajectoryType::RigidBody:
				writer.Write(traj.angle.EncodeRotation());
				writer.Write(traj.angularVelocity);
				break;
			case game::TrajectoryType::Player:
				writer.Write(traj.eulerAngle);
				break;
			default:
				SPRaise("Unknown trajectory type: %d",
						static_cast<int>(traj.type));
		}
	}
	
	enum class PlayerInputFlags {
		None = 0,
		ToolPrimary = 1 << 0,
		ToolSecondary = 1 << 1,
		Chat = 1 << 2,
		Sprint = 1 << 3,
		
		StanceMask = 3 << 6
	};
	
	inline PlayerInputFlags operator|(PlayerInputFlags a, PlayerInputFlags b)
	{
		return static_cast<PlayerInputFlags>(static_cast<int>(a) | static_cast<int>(b));
	}
	inline PlayerInputFlags& operator |=(PlayerInputFlags& a, PlayerInputFlags b) {
		a = a | b;
		return a;
	}
	inline int operator&(PlayerInputFlags a, PlayerInputFlags b)
	{
		return static_cast<int>(a) & static_cast<int>(b);
	}
	
	static PlayerInput DecodePlayerInput(PacketReader& reader) {
		PlayerInput inp;
		auto flags = static_cast<PlayerInputFlags>(reader.ReadByte());
		inp.toolPrimary = flags & PlayerInputFlags::ToolPrimary;
		inp.toolSecondary = flags & PlayerInputFlags::ToolSecondary;
		inp.chat = flags & PlayerInputFlags::Chat;
		inp.sprint = flags & PlayerInputFlags::Sprint;
		inp.stance = static_cast<game::PlayerStance>((flags & PlayerInputFlags::StanceMask) >> 6);
		inp.xmove = static_cast<int8_t>(reader.ReadByte());
		inp.ymove = static_cast<int8_t>(reader.ReadByte());
		return inp;
	}
	
	static void WritePlayerInput(PacketWriter& writer, const PlayerInput& input) {
		auto flags = PlayerInputFlags::None;
		if(input.toolPrimary) flags |= PlayerInputFlags::ToolPrimary;
		if(input.toolSecondary) flags |= PlayerInputFlags::ToolSecondary;
		if(input.chat) flags |= PlayerInputFlags::Chat;
		if(input.sprint) flags |= PlayerInputFlags::Sprint;
		flags |= static_cast<PlayerInputFlags>(static_cast<int>(input.stance) << 6);
		writer.Write(static_cast<uint8_t>(flags));
		writer.Write(static_cast<uint8_t>(input.xmove));
		writer.Write(static_cast<uint8_t>(input.ymove));
	}
	
	static EntityUpdateItem DecodeEntityUpdateItem(PacketReader& reader) {
		EntityUpdateItem item;
		item.entityId = static_cast<uint32_t>(reader.ReadVariableInteger());
		
		auto updates = static_cast<EntityUpdateFlags>(reader.ReadByte());
		
		item.create = updates & EntityUpdateFlags::Create;
		if(item.create) {
			item.type = static_cast<EntityType>(reader.ReadByte());
		}
		
		if(updates & EntityUpdateFlags::Flags) {
			item.flags = FromEntityFlagsValue
			(static_cast<EntityFlagsValue>(reader.ReadByte()));
		}
		
		if(updates & EntityUpdateFlags::Trajectory) {
			item.trajectory = DecodeTrajectory(reader);
		}
		
		if(updates & EntityUpdateFlags::PlayerInput) {
			item.playerInput = DecodePlayerInput(reader);
		}
		
		if(updates & EntityUpdateFlags::BlockColor) {
			item.blockColor = reader.ReadIntColor();
		}
		
		if(updates & EntityUpdateFlags::Health) {
			item.health = reader.ReadByte();
		}
		
		if(updates & EntityUpdateFlags::Skins) {
			uint8_t mask = reader.ReadByte();
			if(mask & 1) item.bodySkin = reader.ReadBytes();
			if(mask & 2) item.weaponSkin1 = reader.ReadBytes();
			if(mask & 4) item.weaponSkin2 = reader.ReadBytes();
			if(mask & 8) item.weaponSkin3 = reader.ReadBytes();
		}
		
		return item;
	}
	
	static void WriteEntityUpdateItem(PacketWriter& writer, const EntityUpdateItem& item) {
		writer.WriteVariableInteger(item.entityId);
		
		auto flags = EntityUpdateFlags::None;
		
		if(item.flags) flags |= EntityUpdateFlags::Flags;
		if(item.trajectory) flags |= EntityUpdateFlags::Trajectory;
		if(item.playerInput) flags |= EntityUpdateFlags::PlayerInput;
		if(item.tool) flags |= EntityUpdateFlags::Trajectory;
		if(item.blockColor) flags |= EntityUpdateFlags::BlockColor;
		if(item.health) flags |= EntityUpdateFlags::Health;
		if(item.weaponSkin1 ||
		   item.weaponSkin2 ||
		   item.weaponSkin3 ||
		   item.bodySkin) flags |= EntityUpdateFlags::Skins;
		
		writer.Write(static_cast<uint8_t>(flags));
		
		if(item.create) {
			writer.Write(static_cast<uint8_t>(item.type));
		}
		
		if(item.flags) {
			writer.Write(static_cast<uint8_t>(ToEntityFlagsValue(*item.flags)));
		}
		
		if(item.trajectory) {
			WriteTrajectory(writer, *item.trajectory);
		}
		
		if(item.playerInput) {
			WritePlayerInput(writer, *item.playerInput);
		}
		
		if(item.tool) {
			writer.Write(static_cast<uint8_t>(*item.tool));
		}
		
		if(item.blockColor) {
			writer.WriteColor(*item.blockColor);
		}
		
		if(item.health) {
			writer.Write(static_cast<uint8_t>(*item.health));
		}
		
		if(item.weaponSkin1 ||
		   item.weaponSkin2 ||
		   item.weaponSkin3 ||
		   item.bodySkin) {
			uint8_t mask = 0;
			if(item.bodySkin) mask |= 1;
			if(item.weaponSkin1) mask |= 2;
			if(item.weaponSkin2) mask |= 4;
			if(item.weaponSkin3) mask |= 8;
			if(item.bodySkin) writer.WriteBytes(*item.bodySkin);
			if(item.weaponSkin1) writer.WriteBytes(*item.weaponSkin1);
			if(item.weaponSkin2) writer.WriteBytes(*item.weaponSkin2);
			if(item.weaponSkin3) writer.WriteBytes(*item.weaponSkin3);
		}
		
	}
	
	Packet *EntityUpdatePacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<EntityUpdatePacket> p(new EntityUpdatePacket());
		PacketReader reader(data);
		
		while(!reader.IsEndOfPacket()) {
			p->items.emplace_back(DecodeEntityUpdateItem(reader));
		}
		
		return p.release();
	}
	
	std::vector<char> EntityUpdatePacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		for(const auto& item: items) {
			WriteEntityUpdateItem(writer, item);
		}
		
		return std::move(writer.ToArray());
	}
	
	
	
	Packet *ClientSideEntityUpdatePacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<ClientSideEntityUpdatePacket> p(new ClientSideEntityUpdatePacket());
		PacketReader reader(data);
		
		p->timestamp = reader.ReadTimeStamp();
		
		while(!reader.IsEndOfPacket()) {
			p->items.emplace_back(DecodeEntityUpdateItem(reader));
		}
		
		return p.release();
	}
	
	std::vector<char> ClientSideEntityUpdatePacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.Write(timestamp);
		
		for(const auto& item: items) {
			WriteEntityUpdateItem(writer, item);
		}
		
		return std::move(writer.ToArray());
	}
	
	
	
	Packet *TerrainUpdatePacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<TerrainUpdatePacket> p(new TerrainUpdatePacket());
		PacketReader reader(data);
		
		IntVector3 cursor(0, 0, 0);
		while(!reader.IsEndOfPacket()) {
			TerrainEdit edit;
			IntVector3 ps;
			ps.x = static_cast<int8_t>(reader.ReadByte());
			if(ps.x == -128) {
				// move cursor value
				cursor.x = static_cast<int>(reader.ReadVariableInteger());
				cursor.y = static_cast<int>(reader.ReadVariableInteger());
				cursor.z = static_cast<int>(reader.ReadVariableInteger());
				ps.x = static_cast<int8_t>(reader.ReadByte());
			}
			ps.y = static_cast<int8_t>(reader.ReadByte());
			ps.z = static_cast<int8_t>(reader.ReadByte());
			ps += cursor;
			edit.position = ps;
			
			uint8_t health = reader.ReadByte();
			if(health != 0) {
				uint32_t color = reader.ReadByte();
				color |= static_cast<uint32_t>(reader.ReadByte()) << 8;
				color |= static_cast<uint32_t>(reader.ReadByte()) << 16;
				color |= static_cast<uint32_t>(health) << 24;
				edit.color = color;
			}
			p->edits.push_back(edit);
		}
		
		return p.release();
	}
	
	std::vector<char> TerrainUpdatePacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		IntVector3 cursor(0, 0, 0);
		for(const auto& edit: edits) {
			IntVector3 diff = edit.position - cursor;
			if(diff.x < -127 || diff.x > 127 ||
			   diff.y < -127 || diff.y > 127 ||
			   diff.z < -127 || diff.z > 127) {
				// move cursor
				cursor = edit.position;
				writer.Write(static_cast<uint8_t>(-128));
				writer.WriteVariableInteger(static_cast<uint64_t>(cursor.x));
				writer.WriteVariableInteger(static_cast<uint64_t>(cursor.y));
				writer.WriteVariableInteger(static_cast<uint64_t>(cursor.z));
				diff = IntVector3(0, 0, 0);
			}
			writer.Write(static_cast<uint8_t>(diff.x));
			writer.Write(static_cast<uint8_t>(diff.y));
			writer.Write(static_cast<uint8_t>(diff.z));
			if(edit.color) {
				auto col = *edit.color;
				writer.Write(static_cast<uint8_t>(col >> 24));
				writer.Write(static_cast<uint8_t>(col));
				writer.Write(static_cast<uint8_t>(col >> 8));
				writer.Write(static_cast<uint8_t>(col >> 16));
			}else{
				writer.Write(static_cast<uint8_t>(0));
			}
		}
		
		return std::move(writer.ToArray());
	}
	
	Packet *EntityEventPacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<EntityEventPacket> p(new EntityEventPacket());
		PacketReader reader(data);
		
		p->entityId = static_cast<uint32_t>(reader.ReadVariableInteger());
		p->type = static_cast<EntityEventType>(reader.ReadByte());
		p->param = reader.ReadVariableInteger();
		
		return p.release();
	}
	
	std::vector<char> EntityEventPacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.WriteVariableInteger(entityId);
		writer.Write(static_cast<uint8_t>(type));
		writer.WriteVariableInteger(param);
		
		return std::move(writer.ToArray());
	}
	
	
	Packet *EntityDiePacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<EntityDiePacket> p(new EntityDiePacket());
		PacketReader reader(data);
		
		p->entityId = static_cast<uint32_t>(reader.ReadVariableInteger());
		p->type = static_cast<EntityDeathType>(reader.ReadByte());
		p->param = reader.ReadVariableInteger();
		
		return p.release();
	}
	
	std::vector<char> EntityDiePacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.WriteVariableInteger(entityId);
		writer.Write(static_cast<uint8_t>(type));
		writer.WriteVariableInteger(param);
		
		return std::move(writer.ToArray());
	}
	
	
	
	Packet *PlayerActionPacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<PlayerActionPacket> p(new PlayerActionPacket());
		PacketReader reader(data);
		
		p->timestamp = reader.ReadTimeStamp();
		p->type = static_cast<EntityEventType>(reader.ReadByte());
		p->param = reader.ReadVariableInteger();
		
		return p.release();
	}
	
	std::vector<char> PlayerActionPacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.Write(timestamp);
		writer.Write(static_cast<uint8_t>(type));
		writer.WriteVariableInteger(param);
		
		return std::move(writer.ToArray());
	}
	
	
	Packet *HitEntityPacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<HitEntityPacket> p(new HitEntityPacket());
		PacketReader reader(data);
		
		p->timestamp = reader.ReadTimeStamp();
		p->entityId = static_cast<uint32_t>(reader.ReadVariableInteger());
		p->type = static_cast<HitType>(reader.ReadByte());
		p->damageType = static_cast<DamageType>(reader.ReadByte());
		p->firePosition = reader.ReadVector3();
		p->hitPosition = reader.ReadVector3();
		
		return p.release();
	}
	
	std::vector<char> HitEntityPacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.Write(timestamp);
		writer.WriteVariableInteger(entityId);
		writer.Write(static_cast<uint8_t>(type));
		writer.Write(static_cast<uint8_t>(damageType));
		writer.Write(firePosition);
		writer.Write(hitPosition);
		
		return std::move(writer.ToArray());
	}
	
	
	
	Packet *HitTerrainPacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<HitTerrainPacket> p(new HitTerrainPacket());
		PacketReader reader(data);
		
		p->timestamp = reader.ReadTimeStamp();
		p->blockPosition = reader.ReadBlockCoord();
		p->damageType = static_cast<DamageType>(reader.ReadByte());
		p->firePosition = reader.ReadVector3();
		p->hitPosition = reader.ReadVector3();
		
		return p.release();
	}
	
	std::vector<char> HitTerrainPacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.Write(timestamp);
		writer.WriteBlockCoord(blockPosition);
		writer.Write(static_cast<uint8_t>(damageType));
		writer.Write(firePosition);
		writer.Write(hitPosition);
		
		return std::move(writer.ToArray());
	}
	
	
	
	
} }
