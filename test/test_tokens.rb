require 'helper'

describe 'Chipper tokens' do
  before do
    @tweet = "@youtube, why is that stories abt @apple on #cnn always get #removed from http://www1.youtube.com/videos/?"
  end

  after do
    Chipper.skip_tokens    nil
  end

  it 'should extract tokens' do
    Chipper.tokens(@tweet).must_equal %w(why that stories abt always get from http www1 youtube com videos)
  end

  it 'should skip tokens' do
    Chipper.skip_tokens(%w(story always from video))
    Chipper.tokens(@tweet).must_equal %w(why that abt get http www1 youtube com)
  end
end
