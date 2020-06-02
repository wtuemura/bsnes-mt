#include "bsnes-mt/strings.h" // MT.
#include "bsnes-mt/utils.h" // MT.
#include "bsnes-mt/windows.h" // MT.

namespace bms = bsnesMt::strings; // MT.

auto Program::openGame(BrowserDialog& dialog) -> string {
  if(!settings.general.nativeFileDialogs) {
    return dialog.openObject();
  }

  BrowserWindow window;
  window.setTitle(dialog.title());
  window.setPath(dialog.path());
  window.setFilters(dialog.filters());
  window.setParent(dialog.alignmentWindow());
  return window.open();
}

auto Program::openFile(BrowserDialog& dialog) -> string {
  if(!settings.general.nativeFileDialogs) {
    return dialog.openFile();
  }

  BrowserWindow window;
  window.setTitle(dialog.title());
  window.setPath(dialog.path());
  window.setFilters(dialog.filters());
  window.setParent(dialog.alignmentWindow());
  return window.open();
}

auto Program::saveFile(BrowserDialog& dialog) -> string {
  if(!settings.general.nativeFileDialogs) {
    return dialog.saveFile();
  }

  BrowserWindow window;
  window.setTitle(dialog.title());
  window.setPath(dialog.path());
  window.setFilters(dialog.filters());
  window.setParent(dialog.alignmentWindow());
  return window.save();
}

auto Program::selectPath() -> string {
  if(!settings.general.nativeFileDialogs) {
    BrowserDialog dialog;
    dialog.setPath(Path::desktop());
    return dialog.selectFolder();
  }

  BrowserWindow window;
  window.setPath(Path::desktop());
  return window.directory();
}

auto Program::showMessage(string text) -> void {
  statusTime = chrono::millisecond();
  statusMessage = text;
}

auto Program::showFrameRate(string text) -> void {
  statusFrameRate = text;
}

auto Program::updateStatus() -> void {
  string message;
  if(chrono::millisecond() - statusTime <= 2000) {
    message = statusMessage;
  }
  if(message != presentation.statusLeft.text()) {
    presentation.statusLeft.setText(message);
  }

  string frameRate;
  if(!emulator->loaded()) {
    frameRate = bms::get("Program.Unloaded").data(); // tr("Unloaded")
  } else if(presentation.frameAdvance.checked() && frameAdvanceLock) {
    frameRate = bms::get("Tools.RunMode.FrameAdvance").data(); // tr("Frame Advance")
  } else if(presentation.pauseEmulation.checked()) {
    frameRate = bms::get("Program.Paused").data(); // tr("Paused")
  } else if(!focused() && inputSettings.pauseEmulation.checked()) {
    frameRate = bms::get("Program.Paused").data(); // tr("Paused")
  } else {
    frameRate = statusFrameRate;
  }
  if(frameRate != presentation.statusRight.text()) {
    presentation.statusRight.setText(frameRate);
  }
}

auto Program::captureScreenshot() -> bool {
  if(emulator->loaded() && screenshot.data) {
    if(auto filename = screenshotPath()) {
      //RGB555 -> RGB888
      image capture{0, 16, 0x8000, 0x7c00, 0x03e0, 0x001f};
      capture.copy(screenshot.data, screenshot.pitch, screenshot.width, screenshot.height);
      capture.transform(0, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);

      //normalize pixel aspect ratio to 1:1
      if(capture.width() == 512 && capture.height() == 240) capture.scale(512, 480, false);  //hires
      if(capture.width() == 256 && capture.height() == 480) capture.scale(512, 480, false);  //interlace

      auto data = capture.data();
      auto pitch = capture.pitch();
      auto width = capture.width();
      auto height = capture.height();

      if(!presentation.showOverscanArea.checked()) {
        if(height == 240) data +=  8 * pitch, height -= 16;
        if(height == 480) data += 16 * pitch, height -= 32;
      }

      bsnesMt::saveBgraArrayAsPngImage(data, width, height, filename.get()); // MT.

      //if(Encode::BMP::create(filename, data, width << 2, width, height, /* alpha = */ false)) { // Commented-out by MT.
        showMessage({bms::get("Program.CapturedScreenshot").data(), " [", Location::file(filename), "]"}); // Captured screenshot
        return true;
      //} // Commented-out by MT.
    }
  }
  return false;
}

auto Program::inactive() -> bool {
  if(locked()) return true;
  if(!emulator->loaded()) return true;
  if(presentation.pauseEmulation.checked()) return true;
  if(presentation.frameAdvance.checked() && frameAdvanceLock) return true;
  if(!focused() && inputSettings.pauseEmulation.checked()) return true;
  return false;
}

auto Program::focused() -> bool {
  //full-screen mode creates its own top-level window: presentation window will not have focus
  if(video.fullScreen() || presentation.focused()) {
    mute &= ~Mute::Unfocused;
    return true;
  } else {
    if(settings.audio.muteUnfocused) {
      mute |= Mute::Unfocused;
    } else {
      mute &= ~Mute::Unfocused;
    }
    return false;
  }
}
