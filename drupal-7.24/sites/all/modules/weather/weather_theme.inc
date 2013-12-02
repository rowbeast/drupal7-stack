<?php

/**
 * @file
 * Prepare themed weather output.
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
 * Custom theme function for preprocessing the weather display.
 */
function theme_weather_theming($variables) {
  $display = $variables['display'];
  $location = $variables['location'];
  $metar = $variables['metar'];
  // Set up variables which will be needed in the templates.
  $weather = new stdClass();
  $weather->real_name = check_plain($location->real_name);
  $weather->condition = weather_format_sky_condition($metar);
  // Support a custom image directory. If the variable is not set or the specified
  // file is not available, fall back to the default images of the module.
  $path = variable_get('weather_image_directory', '');
  $image = file_default_scheme() . '://' . $path . '/' . $metar->image . '.png';
  if (!is_readable($image)) {
    $image = drupal_get_path('module', 'weather') . '/images/' . $metar->image . '.png';
  }
  $size = getimagesize($image);
  // Prepare the <img> tag
  $weather->image = theme('image', array(
    'path' => $image,
    'width' => $size[0],
    'height' => $size[1],
    'alt' => $weather->condition,
    'title' => $weather->condition,
    'attributes' => array('class' => 'weather-image')
  ));
  // Set up optional variables which might be displayed.
  if (!empty($display->settings['data'])) {
    $show = $display->settings['data'];
    $units = $display->units;
    if (!empty($show['temperature']) and isset($metar->temperature)) {
      $weather->temperature = weather_format_temperature($metar->temperature, $units['temperature']);
      if (!empty($display->settings['show_windchill']) and isset($metar->wind_speed)) {
        $weather->windchill = weather_format_windchill($metar->temperature, $metar->wind_speed, $units['temperature']);
      }
    }
    if (!empty($show['wind']) and isset($metar->wind_direction)) {
      // Set gusts speed to 0 if the database returned NULL.
      if (empty($metar->wind_gusts)) {
        $metar->wind_gusts = 0;
      }
      $weather->wind = weather_format_wind($metar->wind_speed, $metar->wind_direction,
        $metar->wind_gusts, $display->settings, $units['windspeed']);
    }
    if (!empty($show['pressure']) and isset($metar->pressure)) {
      $weather->pressure = weather_format_pressure($metar->pressure, $units['pressure']);
    }
    if (!empty($show['humidity']) and isset($metar->dewpoint) and isset($metar->temperature)) {
      $weather->rel_humidity = weather_format_relative_humidity($metar->temperature, $metar->dewpoint);
    }
    if (!empty($show['visibility']) and isset($metar->visibility)) {
      $weather->visibility = weather_format_visibility($metar->visibility, $units['distance']);
    }
    if (!empty($show['suninfo'])) {
      $suninfo = weather_format_suninfo($metar->sunrise_on, $metar->sunset_on);
      $weather->sunrise = $suninfo['sunrise'];
      $weather->sunset = $suninfo['sunset'];
    }
    if (!empty($show['metar']) and isset($metar->raw)) {
      $weather->metar = $metar->raw;
    }
    if (isset($metar->reported_on)) {
      $weather->reported_on = format_date($metar->reported_on);
    }
  }
  // If the original location is not identical to the weather station, show
  // information about which METAR station has been used for weather data.
  if (isset($location->distance)) {
    $weather->station = weather_format_nearest_station(
      $location, $display->units['distance'], $display->settings
    );
  }
  // Use compact block, if desired
  if (!empty($display->settings['show_compact_block'])) {
    return theme('weather_compact', array('weather' => $weather));
  }
  else {
    return theme('weather', array('weather' => $weather));
  }
}

/**
 * Format sky condition and weather phenomena (rain, drizzle, snow, ...).
 *
 * @param object $metar
 *   METAR data object.
 */
function weather_format_sky_condition($metar) {
  if (empty($metar) or !isset($metar->sky_condition)) {
    return t('No data');
  }
  // Sky condition.
  switch ($metar->sky_condition) {
    case 'clear': $result[] = t('Clear sky'); break;
    case 'few': $result[] = t('Few clouds'); break;
    case 'scattered': $result[] = t('Scattered clouds'); break;
    case 'broken': $result[] = t('Broken clouds'); break;
    case 'overcast': $result[] = t('Overcast'); break;
    case 'no significant clouds': $result[] = t('No significant clouds'); break;
  }
  // Weather phenomena.
  if (isset($metar->phenomena)) {
    $phenomena = explode(', ', $metar->phenomena);
    foreach ($phenomena as $phenomen) {
      switch ($phenomen) {
        // Obscuration.
        case 'mist': $result[] = t('mist'); break;
        case 'shallow fog': $result[] = t('shallow fog'); break;
        case 'partial fog': $result[] = t('partial fog'); break;
        case 'patches of fog': $result[] = t('patches of fog'); break;
        case 'fog': $result[] = t('fog'); break;
        case 'smoke': $result[] = t('smoke'); break;
        // Precipitation, rain.
        case 'light rain showers': $result[] = t('light rain showers'); break;
        case 'light freezing rain': $result[] = t('light freezing rain'); break;
        case 'light rain': $result[] = t('light rain'); break;
        case 'heavy rain showers': $result[] = t('heavy rain showers'); break;
        case 'heavy freezing rain': $result[] = t('heavy freezing rain'); break;
        case 'heavy rain': $result[] = t('heavy rain'); break;
        case 'rain showers': $result[] = t('rain showers'); break;
        case 'freezing rain': $result[] = t('freezing rain'); break;
        case 'rain': $result[] = t('rain'); break;
        // Precipitation, drizzle.
        case 'light freezing drizzle': $result[] = t('light freezing drizzle'); break;
        case 'light drizzle': $result[] = t('light drizzle'); break;
        case 'heavy freezing drizzle': $result[] = t('heavy freezing drizzle'); break;
        case 'heavy drizzle': $result[] = t('heavy drizzle'); break;
        case 'freezing drizzle': $result[] = t('freezing drizzle'); break;
        case 'drizzle': $result[] = t('drizzle'); break;
        // Precipitation, snow.
        case 'light blowing snow': $result[] = t('light blowing snow'); break;
        case 'light low drifting snow': $result[] = t('light low drifting snow'); break;
        case 'light snow showers': $result[] = t('light snow showers'); break;
        case 'light snow': $result[] = t('light snow'); break;
        case 'heavy blowing snow': $result[] = t('heavy blowing snow'); break;
        case 'heavy low drifting snow': $result[] = t('heavy low drifting snow'); break;
        case 'heavy snow showers': $result[] = t('heavy snow showers'); break;
        case 'heavy snow': $result[] = t('heavy snow'); break;
        case 'blowing snow': $result[] = t('blowing snow'); break;
        case 'low drifting snow': $result[] = t('low drifting snow'); break;
        case 'snow showers': $result[] = t('snow showers'); break;
        case 'snow': $result[] = t('snow'); break;
      }
    }
  }
  return implode(", ", $result);
}

/**
 * Converts temperatures.
 *
 * @param int $temperature
 *   Temperature in degree celsius.
 * @param string $unit
 *   Unit to be returned (celsius, fahrenheit, ...).
 *
 * @return string
 *   Formatted representation in the desired unit.
 */
function weather_format_temperature($temperature, $unit) {
  // Do the calculation
  $fahrenheit = (int) ($temperature * 9 / 5) + 32;
  // Format the temperature
  if ($unit == 'fahrenheit') {
    $result = t('!temperature&thinsp;°F', array('!temperature' => $fahrenheit));
  }
  elseif ($unit == 'celsiusfahrenheit') {
    $result = t('!temperature_c&thinsp;°C / !temperature_f&thinsp;°F',
      array(
        '!temperature_c' => $temperature,
        '!temperature_f' => $fahrenheit
      )
    );
  }
  elseif ($unit == 'fahrenheitcelsius') {
    $result = t('!temperature_f&thinsp;°F / !temperature_c&thinsp;°C',
      array(
        '!temperature_f' => $fahrenheit,
        '!temperature_c' => $temperature
      )
    );
  }
  else {
    // Default to metric units.
    $result = t('!temperature&thinsp;°C', array('!temperature' => $temperature));
  }
  return preg_replace("/([^ ]*)&thinsp;([^ ]*)/",
    '<span style="white-space:nowrap;">\1&thinsp;\2</span>', $result);
}

/**
 * Calculates windchill.
 *
 * Windchill temperature is only defined for temperatures at or below
 * 10 °C (50 °F) and wind speeds above 4.8 km/h (3 mph). Bright sunshine
 * may increase the wind chill temperature.
 * @link http://en.wikipedia.org/wiki/Wind_chill
 *
 * @param int $temperature
 *   Temperature in degree celsius.
 * @param int $wind_speed
 *   Wind speed in km/h.
 * @param string $unit
 *   Unit to be returned (celsius, fahrenheit, ...).
 *
 * @return string
 *   Formatted representation in the desired unit. If the windchill is not
 *   defined for the current conditions, returns NULL.
 */
function weather_format_windchill($temperature, $wind_speed, $unit) {
  if (($temperature <= 10) and ($wind_speed >= 4.8)) {
    // Calculate windchill (in degree Celsius).
    $windchill = round(13.12 + 0.6215 * $temperature -
      11.37 * pow($wind_speed, 0.16) +
      0.3965 * $temperature * pow($wind_speed, 0.16), 1);
    $result = weather_format_temperature($windchill, $unit);
  }
  else {
    // Windchill is not defined for current conditions.
    $result = NULL;
  }
  return $result;
}

/**
 * Convert wind.
 *
 * @param int $wind_speed
 *   Wind speed in km/h.
 * @param string $wind_direction
 *   Wind direction ('variable' or compass bearings, e.g. '080')
 * @param int $wind_gusts
 *   Speed of wind gusts
 * @param array $settings
 *   Settings for displaying (abbreviated directions, with compass bearing)
 * @param string $unit
 *   Unit to be returned (km/h, knots, meter/s, ...).
 *
 * @return string
 *   Formatted representation in the desired unit.
 */
function weather_format_wind($wind_speed, $wind_direction, $wind_gusts, $settings, $unit) {
  // Settings for wind directions.
  $abbreviated = FALSE;
  if (!empty($settings['show_abbreviated_directions'])) {
    $abbreviated = TRUE;
  }
  $exact_degree = FALSE;
  if (!empty($settings['show_directions_degree'])) {
    $exact_degree = TRUE;
  }

  // Shortcut for a special case.
  if ($wind_speed == 0 and $wind_direction == 0) {
    return t('Calm');
  }
  // Handle variable directions
  if ($wind_direction == 'variable') {
    // TRANSLATORS: This is a 'variable' wind direction.
    $result[] = t('Variable');
  }
  elseif (drupal_strlen($wind_direction) > 3) {
    // The wind is variable with compass bearings, e.g. 090-130
    list($start, $end) = explode('-', $wind_direction);
    $start_text = weather_bearing_to_text($start, $abbreviated);
    $end_text = weather_bearing_to_text($end, $abbreviated);
    if ($exact_degree) {
      $result[] = t('Variable from !direction_a (!degree_a°) to !direction_b (!degree_b°)',
        array(
          '!direction_a' => $start_text, '!degree_a' => $start,
          '!direction_b' => $end_text, '!degree_b' => $end,
        )
      );
    }
    else {
      $result[] = t('Variable from !direction_a to !direction_b',
        array('!direction_a' => $start_text, '!direction_b' => $end_text)
      );
    }
  }
  else {
    // The wind has only one exact direction.
    $text = weather_bearing_to_text($wind_direction, $abbreviated);
    if ($exact_degree) {
      $result[] = t('!direction (!degree°)',
        array('!direction' => $text, '!degree' => $wind_direction)
      );
    }
    else {
      $result[] = $text;
    }
  }

  // Set up the wind speed.
  if ($unit == 'mph') {
    // Convert into mph.
    $speed = round($wind_speed * 0.62137, 1);
    $result[] = t('!speed&thinsp;mph', array('!speed' => $speed));
    $gusts_speed = t('!speed&thinsp;mph', array('!speed' => round($wind_gusts * 0.62137, 1)));
  }
  elseif ($unit == 'knots') {
    // Convert into knots.
    $speed = round($wind_speed * 0.53996, 1);
    $result[] = t('!speed&thinsp;knots', array('!speed' => $speed));
    $gusts_speed = t('!speed&thinsp;knots', array('!speed' => round($wind_gusts * 0.53996, 1)));
  }
  elseif ($unit == 'mps') {
    // Convert into meter per second.
    $speed = round($wind_speed * 0.27778, 1);
    $result[] = t('!speed&thinsp;meter/s', array('!speed' => $speed));
    $gusts_speed = t('!speed&thinsp;meter/s', array('!speed' => round($wind_gusts * 0.27778, 1)));
  }
  elseif ($unit == 'beaufort') {
    // Convert into Beaufort.
    $number = weather_calculate_beaufort($wind_speed);
    $result[] = t('Beaufort !number', array('!number' => $number));
    $gusts_speed = t('Beaufort !number', array('!number' => weather_calculate_beaufort($wind_gusts)));
  }
  else {
    // Default to metric units.
    $result[] = t('!speed&thinsp;km/h', array('!speed' => $wind_speed));
    $gusts_speed = t('!speed&thinsp;km/h', array('!speed' => $wind_gusts));
  }

  // Set up gusts, if applicable.
  if (!empty($wind_gusts)) {
    $result[] = t('gusts up to !speed', array('!speed' => $gusts_speed));
  }
  $tmp = preg_replace("/([^ ]*)&thinsp;([^ ]*)/",
    '<span style="white-space:nowrap;">\1&thinsp;\2</span>', $result);
  return implode(', ', $tmp);
}

/**
 * Converts a compass bearing to a text direction.
 *
 * This function can be used to get a text representation of a compass
 * bearing (e.g. 0° North, 86° East, ...).
 *
 * @param string $bearing
 *   Compass bearing in degrees.
 * @param boolean $abbreviated
 *   If true, return abbreviated directions (N, NNW) instead of
 *   full text (North, North-Northwest). Defaults to full text directions.
 *
 * @return string
 *   Formatted representation.
 */
function weather_bearing_to_text($bearing, $abbreviated=FALSE) {
  // Determine the sector. This works for 0° up to 348.75°
  // If the bearing was greater than 348.75°, perform a wrap (%16)
  $sector = floor(($bearing + 11.25) / 22.5) % 16;

  if (!$abbreviated) {
    $direction = array(
      t('North'),
      t('North-Northeast'),
      t('Northeast'),
      t('East-Northeast'),
      t('East'),
      t('East-Southeast'),
      t('Southeast'),
      t('South-Southeast'),
      t('South'),
      t('South-Southwest'),
      t('Southwest'),
      t('West-Southwest'),
      t('West'),
      t('West-Northwest'),
      t('Northwest'),
      t('North-Northwest')
    );
  }
  else {
    $direction = array(
      t('N'),
      t('NNE'),
      t('NE'),
      t('ENE'),
      t('E'),
      t('ESE'),
      t('SE'),
      t('SSE'),
      t('S'),
      t('SSW'),
      t('SW'),
      t('WSW'),
      t('W'),
      t('WNW'),
      t('NW'),
      t('NNW')
    );
  }
  return $direction[$sector];
}

/**
 * Calculate Beaufort wind scale for given wind speed.
 *
 * @link http://en.wikipedia.org/wiki/Beaufort_scale
 *
 * @param int $wind_speed
 *   Wind speed in km/h.
 *
 * @return int
 *   Beaufort number.
 */
function weather_calculate_beaufort($wind_speed) {
  if ($wind_speed >= 120) return 12;
  if ($wind_speed >= 103) return 11;
  if ($wind_speed >= 88) return 10;
  if ($wind_speed >= 76) return 9;
  if ($wind_speed >= 63) return 8;
  if ($wind_speed >= 51) return 7;
  if ($wind_speed >= 40) return 6;
  if ($wind_speed >= 30) return 5;
  if ($wind_speed >= 20) return 4;
  if ($wind_speed >= 12) return 3;
  if ($wind_speed >= 7) return 2;
  if ($wind_speed >= 1) return 1;
  return 0;
}

/**
 * Convert pressure.
 *
 * @param int $pressure
 *   Pressure in hPa.
 * @param string $unit
 *   Unit to be returned (e.g. inHg, mmHg, hPa, kPa).
 *
 * @return string
 *   Formatted representation.
 */
function weather_format_pressure($pressure, $unit) {
  if ($unit == 'inhg') {
    $result = t('!pressure&thinsp;inHg',
      array('!pressure' => round($pressure * 0.02953, 2)));
  }
  elseif ($unit == 'mmhg') {
    $result = t('!pressure&thinsp;mmHg',
      array('!pressure' => round($pressure * 0.75006, 0)));
  }
  elseif ($unit == 'kpa') {
    $result = t('!pressure&thinsp;kPa',
      array('!pressure' => round($pressure / 10, 1)));
  }
  else {
    // default to metric units
    $result = t('!pressure&thinsp;hPa', array('!pressure' => $pressure));
  }
  return preg_replace("/([^ ]*)&thinsp;([^ ]*)/",
    '<span style="white-space:nowrap;">\1&thinsp;\2</span>', $result);
}

/**
 * Calculate relative humidity.
 *
 * @param int $temperature
 *   Temperature (degree Celsius).
 * @param int $dewpoint
 *   Dewpoint (degree Celsius).
 *
 * @return string
 *   Formatted representation.
 */
function weather_format_relative_humidity($temperature, $dewpoint) {
  $e = 6.11 * pow(10, (7.5 * $dewpoint / (237.7 + $dewpoint)));
  $es = 6.11 * pow(10, (7.5 * $temperature / (237.7 + $temperature)));
  $result = t('!rel_humidity&thinsp;%',
    array('!rel_humidity' => max(0, min(100, round(100 * ($e / $es), 0)))));
  return preg_replace("/([^ ]*)&thinsp;([^ ]*)/",
    '<span style="white-space:nowrap;">\1&thinsp;\2</span>', $result);
}

/**
 * Convert visibility.
 *
 * @param int $visibility
 *   Visibility in meter.
 * @param string $unit
 *   Unit to be returned (e.g. UK miles, km).
 *
 * @return string
 *   Formatted representation.
 */
function weather_format_visibility($visibility, $unit) {
  if ($unit == 'miles') {
    $result = t('!visibility&thinsp;mi',
      array('!visibility' => round($visibility / 1609.344, 1)));
  }
  else {
    // Default to metric units.
    $result = t('!visibility&thinsp;km',
      array('!visibility' => round($visibility / 1000, 1)));
  }
  return preg_replace("/([^ ]*)&thinsp;([^ ]*)/",
    '<span style="white-space:nowrap;">\1&thinsp;\2</span>', $result);
}

/**
 * Format sunrise and sunset times.
 *
 * @param int $sunrise
 *   Time of sunrise (UTC) as UNIX timestamp.
 * @param int $sunset
 *   Time of sunset (UTC) as UNIX timestamp.
 *
 * @return string
 *   Formatted representation.
 */
function weather_format_suninfo($sunrise, $sunset) {
  if (is_null($sunrise)) {
    $result['sunrise'] = t('No sunrise today');
    $result['sunset'] = NULL;
  }
  elseif (is_null($sunset)) {
    $result['sunrise'] = NULL;
    $result['sunset'] = t('No sunset today');
  }
  else {
    // Try to extract a time format from the system wide date format.
    $date_format_short = variable_get('date_format_short', 'm/d/Y - H:i');
    preg_match("/[GgHh].*?i(.*?[Aa])?/", $date_format_short, $matches);
    if (isset($matches[0])) {
      $format = $matches[0];
    }
    else {
      $format = 'G:i';
    }
    $time = format_date($sunrise, 'custom', $format);
    $result['sunrise'] = t('Sunrise: !sunrise', array('!sunrise' => $time));
    $time = format_date($sunset, 'custom', $format);
    $result['sunset'] = t('Sunset: !sunset', array('!sunset' => $time));
  }
  return $result;
}

/**
 * Format information about nearest METAR station.
 *
 * @param object $location
 *   Location object.
 * @param string $unit
 *   Unit to be returned (e.g. UK miles, km).
 * @param array $settings
 *   Settings for displaying (abbreviated directions, with compass bearing)
 *
 * @return string
 *   Formatted representation.
 */
function weather_format_nearest_station($location, $unit, $settings) {
  if (!empty($settings['show_abbreviated_directions'])) {
    $bearing = weather_bearing_to_text($location->bearing, TRUE);
  }
  else {
    $bearing = weather_bearing_to_text($location->bearing);
  }
  $exact_degree = FALSE;
  if (!empty($settings['show_directions_degree'])) {
    $exact_degree = TRUE;
  }

  if ($unit == 'miles') {
    $distance = round($location->distance / 1.609344, 1);
    if ($exact_degree) {
      $result = t('!distance&thinsp;mi !bearing (!degree°)', array(
        '!distance' => $distance,
        '!bearing' => $bearing,
        '!degree' => $location->bearing,
      ));
    }
    else {
      $result = t('!distance&thinsp;mi !bearing', array(
        '!distance' => $distance,
        '!bearing' => $bearing,
      ));
    }
  }
  else {
    // Default to metric units.
    $distance = $location->distance;
    if ($exact_degree) {
      $result = t('!distance&thinsp;km !bearing (!degree°)', array(
        '!distance' => $distance,
        '!bearing' => $bearing,
        '!degree' => $location->bearing,
      ));
    }
    else {
      $result = t('!distance&thinsp;km !bearing', array(
        '!distance' => $distance,
        '!bearing' => $bearing,
      ));
    }
  }
  return preg_replace("/([^ ]*)&thinsp;([^ ]*)/",
    '<span style="white-space:nowrap;">\1&thinsp;\2</span>', $result);
}
