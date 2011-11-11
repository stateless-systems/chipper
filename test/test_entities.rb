require 'helper'

describe 'Chipper entities' do
  before do
    @tweet = "@youtube, why is that stories abt @apple on #cnn always get #removed from http://www1.youtube.com/videos/?"
  end

  after do
    Chipper.skip_users    nil
    Chipper.skip_hashtags nil
  end

  it 'should extract users' do
    Chipper.users(@tweet).must_equal %w(@youtube @apple)
  end

  it 'should extract hashtags' do
    Chipper.hashtags(@tweet).must_equal %w(#cnn #removed)
  end

  it 'should extract urls' do
    Chipper.urls(@tweet).must_equal %w(http://www1.youtube.com/videos/?)
  end

  it 'should skip users' do
    Chipper.skip_users(%w(youtube))
    Chipper.users(@tweet).must_equal %w(@apple)
  end

  it 'should skip hashtags' do
    Chipper.skip_hashtags(%w(cnn))
    Chipper.hashtags(@tweet).must_equal %w(#removed)
  end
end
