package com.kii.yankon;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.view.View;
import android.widget.EditText;

import com.larswerkman.holocolorpicker.ColorPicker;
import com.larswerkman.holocolorpicker.SVBar;
import com.larswerkman.holocolorpicker.ValueBar;

import butterknife.ButterKnife;
import butterknife.InjectView;


public class ColorPickerActivity extends Activity implements ColorPicker.OnColorChangedListener, View.OnClickListener {

    private static final String TAG = ColorPickerActivity.class.getSimpleName();

    public static final String EXTRA_COLOR = "color";
    public static final String EXTRA_NAME = "name";
    public static final String EXTRA_ID = "id";

    @InjectView(R.id.picker)
    ColorPicker mPicker;

    @InjectView(R.id.confirm_button)
    View mConfirmButton;

    @InjectView(R.id.svbar)
    SVBar svBar;

    @InjectView(R.id.valuebar)
    ValueBar valueBar;

    @InjectView(R.id.edit)
    EditText mEdit;

    private int mColorValue;
    int id;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_color_picker);
        ButterKnife.inject(this);

        String name = getIntent().getStringExtra(EXTRA_NAME);
        if (name == null) {
            mEdit.setVisibility(View.GONE);
        } else {
            mEdit.setText(name);
        }

        int color = getIntent().getIntExtra(EXTRA_COLOR, Color.WHITE);
        id = getIntent().getIntExtra(EXTRA_ID, -1);
        mPicker.setOldCenterColor(color);
        mPicker.setNewCenterColor(color);
        mPicker.addSVBar(svBar);
        mPicker.addValueBar(valueBar);
        mPicker.setOnColorChangedListener(this);
        mConfirmButton.setOnClickListener(this);
    }

    @Override
    public void onColorChanged(int i) {
        mColorValue = i;
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.confirm_button:
                Intent intent = new Intent();
                intent.putExtra(EXTRA_COLOR, mColorValue);
                intent.putExtra(EXTRA_NAME, mEdit.getText().toString());
                intent.putExtra(EXTRA_ID, id);
                setResult(RESULT_OK, intent);
                finish();
                break;
            default:
                break;
        }
    }
}
