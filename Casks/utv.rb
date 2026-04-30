cask "utv" do
  version "0.0.0" # This will be automatically updated by GitHub Actions
  sha256 "0000000000000000000000000000000000000000000000000000000000000000"

  url "https://github.com/makaisystems/utv/releases/download/#{version}/UTV-#{version}-macOS-arm64.zip"
  name "UTV"
  desc "Lightweight and distributable framecycler and sequence viewer"
  homepage "https://github.com/makaisystems/utv"

  app "UTV.app"

  zap trash: [
    "~/Library/Preferences/com.makaisystems.UTV.plist",
    "~/Library/Saved Application State/com.makaisystems.UTV.savedState",
  ]
end
