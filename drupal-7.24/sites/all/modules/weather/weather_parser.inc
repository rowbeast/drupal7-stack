<?php

/**
 * @file
 * Retrieves and parses raw METAR data and stores result in database.
 *
 * Copyright © 2006-2012 Tobias Quathamer <t.quathamer@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * Parses raw METAR data string and stores results in database.
 *
 * @param string $icao
 *   ICAO code.
 * @param object $metar
 *   METAR data object, may be altered.
 *
 * @return object
 *   METAR data object or FALSE.
 */
function weather_refresh_data($icao, &$metar) {
  $metar_raw_string = weather_retrieve_data($icao);
  // If there's new data available, parse it.
  if (!empty($metar_raw_string)) {
    $metar = weather_parse_metar($metar_raw_string);
    // Calculate the next scheduled update. Use 62 minutes after the
    // reported timestamp, to allow the data to propagate to the server.
    $next_update_on = $metar->reported_on + 62 * 60;
    // However, if the current time is more than 62 minutes
    // over the reported timestamp, do not download on every page request.
    // Therefore, we use 3, 6, 12, and 24 hours to check for updates.
    // From then on, we check once a day for updates.
    if ($next_update_on < REQUEST_TIME) {
      $last_update = $metar->reported_on;
      $hours = 3 * 60 * 60;
      while (($last_update + $hours + 120) < REQUEST_TIME) {
        if ($hours < 86400) {
          $hours = $hours * 2;
        }
        else {
          $hours = $hours + 86400;
        }
      }
      // Add 2 minutes to allow the data to propagate to the server.
      $next_update_on = $last_update + $hours + 120;
    }
    $metar->next_update_on = $next_update_on;
  }
  else {
    // The download has not been successful. Calculate the time of next update
    // according to last tries.
    if (empty($metar)) {
      // There is no entry yet, so this is the first download attempt.
      // Create a new entry with 'nodata' and give some minutes to retry.
      $metar->icao = $icao;
      $metar->reported_on = REQUEST_TIME;
      $metar->next_update_on = REQUEST_TIME + 10 * 60;
      $metar->image = 'nodata';
    }
    else {
      // There has been at least one download attempt. Increase the time of
      // next update to not download every few minutes.
      // If 24 hours are reached, we check once a day for updates.
      // This way, we gracefully handle ICAO codes which do no longer exist.
      $last_update = $metar->reported_on;
      $hours = 3 * 60 * 60;
      while (($last_update + $hours + 120) < REQUEST_TIME) {
        if ($hours < 86400) {
          $hours = $hours * 2;
        }
        else {
          $hours = $hours + 86400;
        }
      }
      // Add 2 minutes to allow the data to propagate to the server.
      $metar->next_update_on = $last_update + $hours + 120;
    }
  }
  // Make sure there's no stale data around
  db_delete('weather_metar')
    ->condition('icao', $icao)
    ->execute();
  drupal_write_record('weather_metar', $metar);
}

/**
 * Retrieve data from http://www.aviationweather.gov/
 *
 * @param string $icao
 *   ICAO code
 *
 * @return string
 *   Raw METAR string or FALSE
 */
function weather_retrieve_data($icao) {
  $metar_raw = FALSE;
  // Specify timeout in seconds
  $timeout = 10;
  $url = 'http://www.aviationweather.gov/adds/metars/?chk_metars=on&station_ids=' . $icao;
  $response = drupal_http_request($url, array('timeout' => $timeout));
  // Extract the valid METAR data from the received webpage.
  if (preg_match("/($icao [0-9]{6}Z [^<]+)/m", $response->data, $matches)) {
    $metar_raw = str_replace("\n", "", $matches[1]);
  }
  // Check for errors.
  if ($metar_raw === FALSE) {
    // Make an entry about this error into the watchdog table.
    watchdog('weather', 'Download location for METAR data is not accessible.',
      array(), WATCHDOG_ERROR);
    // Show a message to users with administration priviledges
    if (user_access('administer custom weather block') or user_access('administer site configuration')) {
      drupal_set_message(t('Download location for METAR data is not accessible.'), 'error');
    }
  }
  return $metar_raw;
}

/**
 * Parses a raw METAR data string.
 *
 * @param string $metar_raw_string
 *   Raw METAR data string.
 *
 * @return object
 *   METAR data object.
 */
function weather_parse_metar($metar_raw_string) {
  // Setup the METAR data object.
  $metar = new stdClass();
  $metar->raw = $metar_raw_string;
  // Extract the date and time in UTC.
  $metar->reported_on = _weather_parse_timestamp($metar_raw_string);
  // Some stations insert a space between the cloud conditions
  // and the altitude, e.g. "FEW 025" instead of "FEW025".
  // Therefore, we scan for such occurences and remove the space.
  $metar_raw_string = preg_replace("/(FEW|SCT|BKN|OVC)\s+([0-9]{3})/",
    '$1$2', $metar->raw);
  // Split string for parsing routines
  $raw_items = preg_split('/\s+/', drupal_strtoupper($metar_raw_string));
  // Run the data through the METAR routines
  foreach ($raw_items as $metar_raw) {
    if (_weather_parse_stop($metar_raw)) break;
    _weather_parse_icao($metar_raw, $metar);
    _weather_parse_sky_condition($metar_raw, $metar);
    _weather_parse_phenomena($metar_raw, $metar);
    _weather_parse_temperature($metar_raw, $metar);
    _weather_parse_wind($metar_raw, $metar);
    _weather_parse_pressure($metar_raw, $metar);
    _weather_parse_visibility($metar_raw, $metar);
  }
  if (isset($metar->phenomena)) {
    $metar->phenomena = implode(', ', $metar->phenomena);
  }
  // Calculate sunrise and sunset times
  _weather_calculate_sunrise_sunset($metar);
  // Set up the image filename.
  _weather_construct_image_filename($metar);
  return $metar;
}

/**
 * Extract timestamp.
 *
 * @param string $metar_raw
 *   Raw METAR data to parse
 *
 * @return string
 *   Datetime string (UTC) for database storage
 */
function _weather_parse_timestamp($metar_raw) {
  // The format to match is DDHHMMZ, e.g. 251720Z
  if (preg_match('/ ([0-9]{2})([0-9]{2})([0-9]{2})Z /', $metar_raw, $matches)) {
    // Construct the date
    $month = gmdate("n");
    $year = gmdate("Y");
    // If the day is larger than the current day, it's most probably
    // a day in the last month.
    if ($matches[1] > gmdate("d")) {
      $month -= 1;
    }
    return gmmktime(
      $matches[2], // hours
      $matches[3], // minutes
      0,           // seconds
      $month,      // month
      $matches[1], // day
      $year        // year
    );
  }
}

/**
 * Decide whether to stop parsing.
 *
 * @param string $metar_raw
 *   Raw METAR data to parse.
 *
 * @return boolean
 *   TRUE or FALSE.
 */
function _weather_parse_stop($metar_raw) {
  if (preg_match('/^(BECMG|TEMPO|NOSIG|RMK)$/', $metar_raw)) {
    return TRUE;
  }
  else {
    return FALSE;
  }
}

/**
 * Extract ICAO code.
 *
 * ICAO = International Civil Aviation Organization, this is a four
 * letter airport code, e. g. EDDH
 *
 * @param string $metar_raw
 *   Raw METAR data to parse.
 * @param object $metar
 *   METAR data object, may be altered.
 */
function _weather_parse_icao($metar_raw, &$metar) {
  if (preg_match('/^([A-Z]{4}|K[A-Z0-9]{3})$/', $metar_raw) and !isset($metar->icao)) {
    $metar->icao = $metar_raw;
  }
}

/**
 * Extract sky condition information.
 *
 * @param string $metar_raw
 *   Raw METAR data to parse.
 * @param object $metar
 *   METAR data object, may be altered.
 */
function _weather_parse_sky_condition($metar_raw, &$metar) {
  // The ordering of cloud covering is as follows:
  // 0, CLR: clear
  // 1, FEW: few
  // 2, SCT: scattered
  // 3, BKN: broken
  // 4, OVC: overcast
  if (preg_match('/^' .
    '(FEW|SCT|BKN|OVC)([0-9]{3})' .
    '(CB|TCU)?(\/\/\/)?' .
    '$/', $metar_raw, $matches)) {
    switch ($matches[1]) {
      case 'CLR':
        $metar->sky_condition_order[] = 0;
        $metar->sky_condition_text[] = 'clear';
        break;
      case 'FEW':
        $metar->sky_condition_order[] = 1;
        $metar->sky_condition_text[] = 'few';
        break;
      case 'SCT':
        $metar->sky_condition_order[] = 2;
        $metar->sky_condition_text[] = 'scattered';
        break;
      case 'BKN':
        $metar->sky_condition_order[] = 3;
        $metar->sky_condition_text[] = 'broken';
        break;
      case 'OVC':
        $metar->sky_condition_order[] = 4;
        $metar->sky_condition_text[] = 'overcast';
        break;
    }
  }
  elseif (preg_match('/^' .
    '(CLR|SKC|CAVOK)' .
    '$/', $metar_raw, $matches)) {
    $metar->sky_condition_order[] = 0;
    $metar->sky_condition_text[] = 'clear';
    // CAVOK implies a visibility of 10 km, there's no extra visibility section.
    if ($matches[1] == 'CAVOK') {
      $metar->visibility = 10000;
    }
  }
  elseif (preg_match('/^' .
    '(NSC|NCD)' .
    '$/', $metar_raw, $matches)) {
    // NSC means no significant clouds,
    // NCD is from automatic stations, no cloud detected
    // The ordering is equivalent to "FEW"
    $metar->sky_condition_order[] = 1;
    $metar->sky_condition_text[] = 'no significant clouds';
  }
  elseif (preg_match('/^' .
    'VV[0-9\/]{3}' .
    '$/', $metar_raw, $matches)) {
    // VV is the vertical visibility, this should be shown as overcast.
    // If no information is available, VV/// will be used.
    $metar->sky_condition_order[] = 4;
    $metar->sky_condition_text[] = 'overcast';
  }
  if (isset($metar->sky_condition_order)) {
    if (isset($metar->current_sky_condition_order)) {
      foreach ($metar->sky_condition_order as $index => $order) {
        if ($order > $metar->current_sky_condition_order) {
          $metar->current_sky_condition_order = $order;
          $metar->sky_condition = $metar->sky_condition_text[$index];
        }
      }
    }
    else {
      $metar->current_sky_condition_order = $metar->sky_condition_order[0];
      $metar->sky_condition = $metar->sky_condition_text[0];
    }
  }
}

/**
 * Extract phenomena information.
 *
 * @param string $metar_raw
 *   Raw METAR data to parse.
 * @param object $metar
 *   METAR data object, may be altered.
 */
function _weather_parse_phenomena($metar_raw, &$metar) {
  // Handle rain
  if (preg_match('/^' .
    '(-|\+|VC)?' .     // Intensity or Proximity
    '(SH|TS|FZ)?' .    // Descriptor
    'RA' .             // Rain
    '$/', $metar_raw, $matches)) {
    // Construct a result array of the following form:
    // [light/heavy] [freezing] [rain] [showers]
    $result[2] = 'rain';
    $intensity = 'moderate';
    if (isset($matches[1])) {
      switch ($matches[1]) {
        case '-':
          $result[0] = 'light';
          $intensity = 'light';
          break;
        case '+':
          $result[0] = 'heavy';
          $intensity = 'heavy';
          break;
      }
    }
    if (isset($matches[2])) {
      switch ($matches[2]) {
        case 'SH': $result[3] = 'showers'; break;
        case 'FZ': $result[1] = 'freezing'; break;
      }
    }
    ksort($result);
    $metar->phenomena[] = implode(' ', $result);
    // Store parts of the weather phenomena for image name construction.
    $metar->image_part['precipitation'] = "$intensity-rain";
  }
  elseif (preg_match('/^' .
    '(-|\+|VC)?' .     // Intensity or Proximity
    '(FZ)?' .          // Descriptor
    'DZ' .             // Drizzle
    '$/', $metar_raw, $matches)) {
    // Construct a result array of the following form:
    // [light/heavy] [freezing] [drizzle]
    $result[2] = 'drizzle';
    $intensity = 'moderate';
    if (isset($matches[1])) {
      switch ($matches[1]) {
        case '-':
          $result[0] = 'light';
          $intensity = 'light';
          break;
        case '+':
          $result[0] = 'heavy';
          $intensity = 'heavy';
          break;
      }
    }
    if (isset($matches[2])) {
      switch ($matches[2]) {
        case 'FZ': $result[1] = 'freezing'; break;
      }
    }
    ksort($result);
    $metar->phenomena[] = implode(' ', $result);
    // Store parts of the weather phenomena for image name construction.
    $metar->image_part['precipitation'] = "$intensity-rain";
  }
  elseif (preg_match('/^' .
    '(-|\+|VC)?' .     // Intensity or Proximity
    '(BL|DR|SH)?' .    // Descriptor
    'SN' .             // Snow
    '$/', $metar_raw, $matches)) {
    // Construct a result array of the following form:
    // [light/heavy] [blowing/low driftig] [snow] [showers]
    $result[2] = 'snow';
    $intensity = 'moderate';
    if (isset($matches[1])) {
      switch ($matches[1]) {
        case '-':
          $result[0] = 'light';
          $intensity = 'light';
          break;
        case '+':
          $result[0] = 'heavy';
          $intensity = 'heavy';
          break;
      }
    }
    if (isset($matches[2])) {
      switch ($matches[2]) {
        case 'BL': $result[1] = 'blowing'; break;
        case 'DR': $result[1] = 'low drifting'; break;
        case 'SH': $result[3] = 'showers'; break;
      }
    }
    ksort($result);
    $metar->phenomena[] = implode(' ', $result);
    // Store parts of the weather phenomena for image name construction.
    $metar->image_part['precipitation'] = "$intensity-snow";
  }
  elseif (preg_match('/^' .
    'BR' .             // Mist
    '$/', $metar_raw, $matches)) {
    $metar->phenomena[] = 'mist';
    // Store parts of the weather phenomena for image name construction.
    $metar->image_part['fog'] = 'fog';
  }
  elseif (preg_match('/^' .
    '(VC|MI|PR|BC)?' . // Proximity (VC) or Descriptor
    'FG' .             // Fog
    '$/', $metar_raw, $matches)) {
    // Construct a result array of the following form:
    // [shallow/partial/patches of] [fog]
    $result[1] = 'fog';
    if (isset($matches[1])) {
      switch ($matches[1]) {
        case 'MI': $result[0] = 'shallow'; break;
        case 'PR': $result[0] = 'partial'; break;
        case 'BC': $result[0] = 'patches of'; break;
      }
    }
    ksort($result);
    $metar->phenomena[] = implode(' ', $result);
    // Store parts of the weather phenomena for image name construction.
    $metar->image_part['fog'] = 'fog';
  }
  elseif (preg_match('/^' .
    'FU' .             // Smoke
    '$/', $metar_raw, $matches)) {
    $metar->phenomena[] = 'smoke';
    // Store parts of the weather phenomena for image name construction.
    $metar->image_part['fog'] = 'fog';
  }
}

/**
 * Extract temperature information.
 *
 * @param string $metar_raw
 *   Raw METAR data to parse.
 * @param object $metar
 *   METAR data object, may be altered.
 */
function _weather_parse_temperature($metar_raw, &$metar) {
  if (preg_match('/^' .
    '(M?[0-9]{2})' .      // Temperature (M indicates negative temperatures)
    '\/' .                // Slash is the separator
    '(M?[0-9]{2}|XX)?' .  // Optional: Dew point temperature, may be 'XX'
    '$/', $metar_raw, $matches)) {
    $metar->temperature = (int) strtr($matches[1], 'M', '-');
    if (isset($matches[2]) and $matches[2] != 'XX') {
      $metar->dewpoint = (int) strtr($matches[2], 'M', '-');
    }
    else {
      $metar->dewpoint = NULL;
    }
  }
}

/**
 * Extract wind information.
 *
 * @param string $metar_raw
 *   Raw METAR data to parse.
 * @param object $metar
 *   METAR data object, may be altered.
 */
function _weather_parse_wind($metar_raw, &$metar) {
  if (preg_match('/^' .
    '([0-9]{3}|VRB)' .    // Wind direction from true north or VRB for variable
    '([0-9]{2,3})' .      // Wind speed, two or three numbers
    '(G([0-9]{2,3}))?' .  // Gusts, two or three digits (optional)
    '(KT|MPS|KMH)' .      // Measurement unit (knots, meter/s, or km/h)
    '$/', $metar_raw, $matches)) {
    if ($matches[1] == 'VRB') {
      $metar->wind_direction = 'variable';
    }
    else {
      $metar->wind_direction = (int) $matches[1];
    }
    $wind_speed = (int) $matches[2];
    $wind_gusts = (int) $matches[4];
    $wind_unit = $matches[5];

    // Ensure that windspeed is in km/h.
    switch ($wind_unit) {
      case 'KT':
        // Convert from knots (1 knot = 1.852 km/h)
        $metar->wind_speed = round($wind_speed * 1.852, 1);
        if ($wind_gusts > 0) {
          $metar->wind_gusts = round($wind_gusts * 1.852, 1);
        }
        break;
      case 'MPS':
        // Convert from meter/s (1 m/s = 3.6 km/h)
        $metar->wind_speed = round($wind_speed * 3.6, 1);
        if ($wind_gusts > 0) {
          $metar->wind_gusts = round($wind_gusts * 3.6, 1);
        }
        break;
      case 'KMH':
        $metar->wind_speed = $wind_speed;
        if ($wind_gusts > 0) {
          $metar->wind_gusts = $wind_gusts;
        }
        break;
    }
  }
  elseif (preg_match('/^' .
    '([0-9]{3})' .        // Variable wind direction, start
    'V' .                 // Marker for "V"ariable
    '([0-9]{3})' .        // Variable wind direction, end
    '$/', $metar_raw, $matches)) {
    $metar->wind_direction = $matches[1] . '-' . $matches[2];
  }
}

/**
 * Extract pressure information.
 *
 * @param string $metar_raw
 *   Raw METAR data to parse.
 * @param object $metar
 *   METAR data object, may be altered.
 */
function _weather_parse_pressure($metar_raw, &$metar) {
  if (preg_match('/^' .
    '(A|Q)([0-9]{4})' .      // Pressure, either inHg (A) or hPa (Q)
    '$/', $metar_raw, $matches)) {
    if ($matches[1] == 'A') {
      // Pressure is given in inch Hg
      $metar->pressure = round(($matches[2] / 100) * 33.8639, 0);
    }
    else {
      // Pressure is given in HektoPascal, hPa
      $metar->pressure = (int) $matches[2];
    }
  }
}

/**
 * Extract visibility information.
 *
 * @param string $metar_raw
 *   Raw METAR data to parse.
 * @param object $metar
 *   METAR data object, may be altered.
 */
function _weather_parse_visibility($metar_raw, &$metar) {
  if (preg_match('/^([0-9])$/', $metar_raw, $matches)) {
    // Special case: A single digit, e.g. in 1 1/2SM
    $metar->visibility_miles = $matches[1];
  }
  elseif (preg_match('/^' .
    '(M?)([0-9])(\/?)([0-9]*)' .
    'SM' .
    '$/', $metar_raw, $matches)) {
    if ($matches[3] == '/') {
      // This is a fractional visibility, we need to convert this
      $visibility = $metar->visibility_miles + $matches[2] / $matches[4];
    }
    else {
      $visibility = $matches[2] . $matches[4];
    }
    $metar->visibility = round($visibility * 1609.344, 0);
  }
  elseif (preg_match('/^([0-9]{4})(NDV)?$/', $metar_raw, $matches)) {
    // NDV means "no directional variation", used by automatic stations
    $metar->visibility = (int) $matches[1];
  }
}

/**
 * Calculate the times of sunrise and sunset.
 *
 * The times are GMT, so it's possible for the sunrise being
 * at 16:48 while the sun sets at 7:06.
 *
 * @param object $metar
 *   METAR data object, may be altered.
 */
function _weather_calculate_sunrise_sunset(&$metar) {
  // Get the coordinates for this weather station.
  $location = db_query("SELECT latitude, longitude FROM {weather_icao} WHERE icao=:icao",
    array(':icao' => $metar->icao))->fetchAssoc();
  $reported_on = $metar->reported_on;
  // Initialize sunrise and sunset times.
  $metar->sunrise_on = NULL;
  $metar->sunset_on = NULL;
  $suninfo = date_sun_info($reported_on, $location['latitude'], $location['longitude']);
  // Handle special cases (no sunrise or sunset at all).
  if ($suninfo['sunrise'] == 0 and $suninfo['sunset'] == 0) {
    // Sun is always below the horizon. To indicate that the sun
    // does not rise, let sunrise_on be NULL and set sunset_on to today.
    $condition = 'night';
    $metar->sunset_on = $metar->reported_on;
  }
  elseif ($suninfo['sunrise'] == 1 and $suninfo['sunset'] == 1) {
    // Sun is always above the horizon. To indicate that the sun
    // does not set, let sunset_on be NULL and set sunrise_on to today.
    $condition = 'day';
    $metar->sunrise_on = $metar->reported_on;
  }
  else {
    // There is a sunrise and a sunset.
    // We don't need the exact second of the sunrise and sunset. Therefore, the
    // times are rounded to the next minute. We add 30 seconds and cut off the
    // seconds part.
    $metar->sunrise_on = round($suninfo['sunrise'] / 60) * 60;
    $metar->sunset_on = round($suninfo['sunset'] / 60) * 60;
    // Correctly handle northern and southern hemisphere.
    if ($suninfo['sunrise'] <= $suninfo['sunset']) {
      // This should be on the northern hemisphere.
      if ($reported_on >= $suninfo['sunrise'] and $reported_on < $suninfo['sunset']) {
        $condition = 'day';
      }
      else {
        $condition = 'night';
      }
    }
    else {
      // This should be on the southern hemisphere.
      if ($reported_on >= $suninfo['sunrise'] or $reported_on <= $suninfo['sunset']) {
        $condition = 'day';
      }
      else {
        $condition = 'night';
      }
    }
  }
  $metar->daytime_condition = $condition;
}

/**
 * Construct filename of the weather image.
 *
 * @param object $metar
 *   METAR data object, may be altered.
 */
function _weather_construct_image_filename(&$metar) {
  // Is there any data available?
  if (empty($metar->sky_condition)) {
    $metar->image = 'nodata';
  }
  else {
    // First part: daytime (day/night).
    $image_part[] = $metar->daytime_condition;
    // Next part: sky condition
    // Handle special case: NSC, we just use few for the display.
    if ($metar->sky_condition == 'no significant clouds') {
      $image_part[] = 'few';
    }
    else {
      $image_part[] = $metar->sky_condition;
    }
    // Next part: fog.
    if (isset($metar->image_part['fog'])) {
      $image_part[] = $metar->image_part['fog'];
    }
    // Next part: precipitation.
    if (isset($metar->image_part['precipitation'])) {
      $image_part[] = $metar->image_part['precipitation'];
    }
    $metar->image = implode('-', $image_part);
  }
}
