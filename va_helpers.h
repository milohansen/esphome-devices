#pragma once
#include "esphome.h"

namespace my_va
{

//   const esphome::voice_assistant::Timer EMPTY_TIMER = {
//     .id = "",
//     .name = "",
//     .total_seconds = 0,
//     .remaining_seconds = 0,
//     .is_running = false
// };

  inline void transform_zoom(lv_obj_t * obj, int32_t zoom_level) {
    lv_obj_set_style_transform_zoom(obj, zoom_level, LV_PART_MAIN);
  }

  /**
   * Safely updates timer slots on the main loop to prevent LVGL crashes.
   * @param timer_map_ptr      Pointer to the global timer_map
   * @param slot_containers    Array of 3 LVGL container objects
   * @param slot_arcs          Array of 3 LVGL arc widgets
   * @param slot_labels        Array of 3 LVGL label widgets
   * @param slot_ids           Array of 3 timer ID strings
   * @return                   Vector of timer ID strings after update
   */
  inline std::vector<std::string> update_timer_slots_safely(
      std::map<std::string, esphome::voice_assistant::Timer> *timer_map_ptr,
      esphome::mapping::Mapping<int, _lv_obj_t*> slot_containers,
      esphome::mapping::Mapping<int, _lv_obj_t*> slot_arcs,
      esphome::mapping::Mapping<int, _lv_obj_t*> slot_labels,
      int count,
      int count_change)
  {
    std::vector<std::string> new_slot_ids;

    if (timer_map_ptr == nullptr)
    {
      ESP_LOGW("my_va", "update_timer_slots_safely: null pointer detected");
      return new_slot_ids;
    }

    int i = 0;
    // Copy timer data to avoid race conditions
    std::vector<esphome::voice_assistant::Timer> timer_snapshot;
    for (auto &pair : *timer_map_ptr)
    {
      if (pair.second.is_active)
      {
        timer_snapshot.push_back(pair.second);
        if (i < 3)
        {
          new_slot_ids.push_back(pair.second.id);
          i++;
        }
      }
    }
    // ESP_LOGD("my_va", "update_timer_slots_safely: copied %d active timers, prev_used_slots=%d", timer_snapshot.size(), prev_used_slots);

    // Defer to main loop for thread safety with copied data
    App.scheduler.set_timeout(nullptr, "update_timers", 0, [timer_snapshot, slot_containers, slot_arcs, slot_labels, count, count_change]()
                              {
      ESP_LOGD("my_va", "update_timer_slots_safely: count=%d, count_change=%d", count, count_change);
      // Update UI with snapshot data
      for (int i = 0; i < 3; i++)
      {
        if (i < timer_snapshot.size() && slot_containers.get(i) && slot_arcs.get(i) && slot_labels.get(i))
        {
          auto &t = timer_snapshot[i];
          // ESP_LOGD("my_va", "update_timer_slots_safely: updating slot %d with timer %s", i, t.id.c_str());
          lv_obj_clear_flag(slot_containers.get(i), LV_OBJ_FLAG_HIDDEN);
          
          int mins = t.seconds_left / 60;
          int secs = t.seconds_left % 60;
          lv_label_set_text_fmt(slot_labels.get(i), "%d:%02d", mins, secs);
          
          float pct = (float)t.seconds_left / (float)t.total_seconds * 100.0f;
          lv_arc_set_value(slot_arcs.get(i), (int)pct);
          
          // ESP_LOGD("my_va", "Slot %d updated safely: %s - %d:%02d", i, t.id.c_str(), mins, secs);
          // ESP_LOGD("my_va", "Slot %d updated safely: %s", i, t.to_string().c_str());
        }
        else if (i < (count - count_change))
        {
          if (slot_containers.get(i))
          {
            ESP_LOGD("my_va", "Hiding slot container %d", i);
            lv_obj_add_flag(slot_containers.get(i), LV_OBJ_FLAG_HIDDEN);
          }
        }
      } });

    return new_slot_ids;
  }

  // /**
  //  * Safely requests a timer cancellation on the main loop to prevent stack crashes.
  //  * @param va_ptr   A pointer to the VoiceAssistant object (e.g., id(va))
  //  * @param timer_id The UUID string of the timer to cancel
  //  */
  // void cancel_timer_safely(esphome::voice_assistant::VoiceAssistant *va_ptr, std::string timer_id)
  // {
  //   ESP_LOGD("my_va", "cancel_timer_safely: Requesting cancel for timer: %s", timer_id.c_str());
  //   if (va_ptr == nullptr || timer_id.empty())
  //   {
  //     return;
  //   }

  //   // Defer the work to the main App scheduler
  //   App.scheduler.set_timeout(va_ptr, "cancel_timer", 0, [va_ptr, timer_id]()
  //                             {
  //       auto req = esphome::api::VoiceAssistantTimerEventResponse();
  //       req.timer_id = timer_id;
  //       req.event_type = esphome::api::enums::VOICE_ASSISTANT_TIMER_CANCELLED;
        
  //       va_ptr->on_timer_event(req);
        
  //       ESP_LOGD("my_va", "Successfully requested cancel for timer: %s", timer_id.c_str()); });
  // }
} // namespace my_va