/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __ISM433_APP_H__
#define __ISM433_APP_H__

#include "ui_widget.hpp"
#include "ui_navigation.hpp"
#include "ui_receiver.hpp"
#include "ui_rssi.hpp"
#include "ui_channel.hpp"
#include "app_settings.hpp"
#include "event_m0.hpp"

#include "log_file.hpp"

#include "recent_entries.hpp"

#include "ism_packet.hpp"

namespace std {

constexpr bool operator==(const ism::TransponderID& lhs, const ism::TransponderID& rhs) {
	return (lhs.value() == rhs.value());
}

} /* namespace std */

struct ISMRecentEntry {
	using Key = std::pair<ism::Reading::Type, ism::TransponderID>;

	static const Key invalid_key;

	ism::Reading::Type type { invalid_key.first };
	ism::TransponderID id { invalid_key.second };

	size_t received_count { 0 };

	Optional<Pressure> last_pressure { };
	Optional<Temperature> last_temperature { };
	Optional<ism::Flags> last_flags { };

	ISMRecentEntry(
		const Key& key
	) : type { key.first },
		id { key.second }
	{
	}

	Key key() const {
		return { type, id };
	}

	void update(const ism::Reading& reading);
};

using ISMRecentEntries = RecentEntries<ISMRecentEntry>;

class ISMLogger {
public:
	Optional<File::Error> append(const std::filesystem::path& filename) {
		return log_file.append(filename);
	}
	
	void on_packet(const ism::Packet& packet, const uint32_t target_frequency);

private:
	LogFile log_file { };
};

namespace ui {

using ISMRecentEntriesView = RecentEntriesView<ISMRecentEntries>;

class ISMAppView : public View {
public:
	ISMAppView(NavigationView& nav);
	~ISMAppView();

	void set_parent_rect(const Rect new_parent_rect) override;

	// Prevent painting of region covered entirely by a child.
	// TODO: Add flag to View that specifies view does not need to be cleared before painting.
	void paint(Painter&) override { };

	void focus() override;
	
	std::string title() const override { return "ISM RX"; };

private:
	static constexpr uint32_t initial_target_frequency = 315000000;
	static constexpr uint32_t sampling_rate = 2457600;
	static constexpr uint32_t baseband_bandwidth = 1750000;

	// app save settings
	std::app_settings 		settings { }; 		
	std::app_settings::AppSettings 	app_settings { };

	MessageHandlerRegistration message_handler_packet {
		Message::ID::ISMPacket,
		[this](Message* const p) {
			const auto message = static_cast<const ISMPacketMessage*>(p);
			const ism::Packet packet { message->packet, message->signal_type };
			this->on_packet(packet);
		}
	};

	static constexpr ui::Dim header_height = 1 * 16;

	ui::Rect view_normal_rect { };

	RSSI rssi {
		{ 21 * 8, 0, 6 * 8, 4 },
	};

	Channel channel {
		{ 21 * 8, 5, 6 * 8, 4 },
	};

	OptionsField options_band {
		{ 0 * 8, 0 * 16 },
		3,
		{
			{ "315", 315000000 },
			{ "433", 433920000 },
		}
	};

	OptionsField options_pressure {
		{ 5 * 8, 0 * 16 },
		3,
		{
			{ "kPa", 0 },
			{ "PSI", 1 }
		}
	};

	OptionsField options_temperature {
		{ 9 * 8, 0 * 16 },
		1,
		{
			{ "C", 0 },
			{ "F", 1 }
		}
	};

	RFAmpField field_rf_amp {
		{ 13 * 8, 0 * 16 }
	};

	LNAGainField field_lna {
		{ 15 * 8, 0 * 16 }
	};

	VGAGainField field_vga {
		{ 18 * 8, 0 * 16 }
	};

	ISMRecentEntries recent { };
	std::unique_ptr<ISMLogger> logger { };

	const RecentEntriesColumns columns { {
		{ "Tp", 2 },
		{ "ID", 8 },
		{ "Pres", 4 },
		{ "Temp", 4 },
		{ "Cnt", 3 },
		{ "Fl", 2 },
	} };
	ISMRecentEntriesView recent_entries_view { columns, recent };

	uint32_t target_frequency_ = initial_target_frequency;

	void on_packet(const ism::Packet& packet);
	void on_show_list();
	void update_view();

	void on_band_changed(const uint32_t new_band_frequency);

	uint32_t target_frequency() const;
	void set_target_frequency(const uint32_t new_value);

	uint32_t tuning_frequency() const;
};

} /* namespace ui */

#endif/*__ISM433_APP_H__*/
